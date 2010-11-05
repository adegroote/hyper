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
			if (it == map.end()) 
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
			std::cout << "receive ping message " << p.value << std::endl;
			return boost::mpl::void_();
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

	io_s.run();
}
