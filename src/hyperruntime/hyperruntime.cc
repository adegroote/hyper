#include <network/nameserver.hh>

using namespace hyper;

namespace details {
	typedef boost::mpl::vector<network::request_name,
							   network::register_name,
							   network::ping
							   > input_msg;
	typedef boost::mpl::vector<network::request_name_answer,
								network::register_name_answer,
								boost::mpl::void_ 
							    > output_msg;

	typedef boost::make_variant_over<input_msg>::type input_variant;
	typedef boost::make_variant_over<output_msg>::type output_variant;

	struct ability_context 
	{
		network::ns::addr_storage addr;
		int timeout_occurence;
	};
	
	typedef std::map<std::string, ability_context> runtime_map;

	struct runtime_visitor : public boost::static_visitor<output_variant>
	{
		runtime_map &map;
		boost::shared_mutex& m;
		network::tcp::ns_port_generator& gen;

		runtime_visitor(runtime_map& map_, boost::shared_mutex& m_,
						network::tcp::ns_port_generator& gen_) :
			map(map_), m(m_), gen(gen_) 
		{}

		output_variant operator() (const network::request_name& r) const
		{
			std::cerr << "receiving name request : " << r << std::endl;

			boost::shared_lock<boost::shared_mutex> lock(m);
			runtime_map::iterator it = map.find(r.name);

			network::request_name_answer res_msg;
			res_msg.name = r.name;
			res_msg.success = (it != map.end());
			if (it != map.end()) 
				res_msg.endpoint = it->second.addr.tcp_endpoint;

			std::cerr << "answering to name request : " << res_msg << std::endl;
			return res_msg;
		}

		output_variant operator() (const network::register_name& r) const
		{
			using namespace boost::asio;

			std::cerr << "receiving name register request : " << r << std::endl;
			ability_context ctx;
			ctx.addr.tcp_endpoint = ip::tcp::endpoint(ip::address_v4::any(), gen.get());
			ctx.timeout_occurence = 0;

			boost::upgrade_lock<boost::shared_mutex> lock(m);
			runtime_map::iterator it = map.find(r.name);
			bool already_here = (it != map.end());
			if (!already_here) {
				map.insert(std::make_pair(r.name, ctx));
			}

			network::register_name_answer res_msg;
			res_msg.name = r.name;
			res_msg.endpoint = ctx.addr.tcp_endpoint;
			res_msg.success = !already_here;

			std::cerr << "answering to name register request : " << res_msg << std::endl;
			return res_msg;
		}

		output_variant operator() (const network::ping& p) const
		{
			boost::upgrade_lock<boost::shared_mutex> lock(m);
			runtime_map::iterator it = map.find(p.name);
			if (it != map.end())
				it->second.timeout_occurence = 0;

			return boost::mpl::void_();
		}
	};

	struct periodic_check
	{
		boost::asio::io_service& io_s_;

		boost::posix_time::time_duration delay_;
		boost::asio::deadline_timer timer_;

		runtime_map& map_;
		boost::shared_mutex& m_;

		periodic_check(boost::asio::io_service& io_s, 
					   boost::posix_time::time_duration delay,
					   runtime_map& map,
					   boost::shared_mutex &m):
			io_s_(io_s), delay_(delay), timer_(io_s_),
			map_(map), m_(m)
		{}

		void handle_timeout(const boost::system::error_code& e)
		{
			if (!e) {
				std::vector<std::string> dead_agents;
				boost::upgrade_lock<boost::shared_mutex> lock(m_);
				runtime_map::iterator it = map_.begin();
				while (it != map_.end())
				{
					if (it->second.timeout_occurence > 2) {
						dead_agents.push_back(it->first);
						map_.erase(it++);
					} else {
						it->second.timeout_occurence++;
						++it;
					}
				}

				/* XXX : send a global msg to tell to alive agents that the
				 * following agents are dead */
				if (! dead_agents.empty()) {
					std::cout << "the following agents seems dead : ";
					std::copy(dead_agents.begin(), dead_agents.end(), 
							  std::ostream_iterator<std::string>(std::cout, " "));
					std::cout << std::endl;
				}

				run();
			}
		}

		void run()
		{
			timer_.expires_from_now(delay_);
			timer_.async_wait(boost::bind(&periodic_check::handle_timeout, 
									  this,
									  boost::asio::placeholders::error));
		}
	};
}


int main()
{
	boost::asio::io_service io_s;

	boost::shared_mutex m;
	details::runtime_map map;
	network::tcp::ns_port_generator gen("5000");
	details::runtime_visitor runtime_vis(map, m, gen);

	typedef network::tcp::server<details::input_msg, 
								  details::output_msg, 
								  details::runtime_visitor
								> tcp_runtime_impl;
	tcp_runtime_impl runtime("localhost", "4242", runtime_vis, io_s);

	details::periodic_check check(io_s, 
								   boost::posix_time::milliseconds(100), 
								   map, m);
						 
	check.run();
	io_s.run();
}
