#include <network/actor_protocol.hh>
#include <network/log.hh>
#include <network/nameserver.hh>

#include <hyperConfig.hh>

using namespace hyper;

#define HYPERRUNTIME_NAME "root"

namespace details {
	typedef boost::mpl::vector<network::request_name,
							   network::register_name,
							   network::request_list_agents,
							   network::ping
							   > input_msg;
	typedef boost::mpl::vector<network::request_name_answer,
								network::register_name_answer,
								network::list_agents,
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

	struct trivial_name_client {
		const runtime_map& map;

		trivial_name_client(const runtime_map& map_) : map(map_) {}

		template <typename Handler>
		void async_resolve(network::name_resolve & solv, Handler handler)
		{
			runtime_map::const_iterator it = map.find(solv.name());
			solv.rna.success = (it != map.end());
			if (it != map.end()) 
				solv.rna.endpoints = it->second.addr.tcp_endpoints;
			handler(boost::system::error_code());
		}
	};

	struct runtime_actor {
		std::string name;
		boost::asio::io_service io_s;
		trivial_name_client name_client;
		network::logger<trivial_name_client> logger;

		runtime_actor(const runtime_map& map) :
					name(HYPERRUNTIME_NAME), 
				    name_client(map),
					logger(io_s, name, "logger", name_client, DEBUG_ALL)
		{}
	};

	typedef hyper::network::actor_client_database<runtime_actor> client_db;

	void handle_new_agent_write(const boost::system::error_code&,
							    boost::shared_ptr<network::inform_new_agent> ptr)
	{
		// let the system release the last reference on ptr
		(void) ptr;
	}

	void handle_list_agents(const boost::system::error_code&,
							boost::shared_ptr<network::list_agents> ptr)
	{
		// let the system release the last reference on ptr
		(void) ptr;
	}

	struct broadcast_inform  {
		boost::shared_ptr<network::inform_new_agent> ptr;
		client_db &db;

		broadcast_inform (boost::shared_ptr<network::inform_new_agent> ptr_,
						 client_db& db) :
			ptr(ptr_), db(db) {}

		void operator() (const std::pair<std::string, ability_context>& p) const
		{
			std::vector<std::string>::const_iterator it;
			it = std::find(ptr->new_agents.begin(), ptr->new_agents.end(),
						   p.first);
			/* Don't send a message to new agent, they know their own existence */
			if (it != ptr->new_agents.end())
				return; 

			db[p.first].async_write(*ptr, 
					boost::bind(&handle_new_agent_write,
						boost::asio::placeholders::error,
						ptr));
		}
	};


	struct runtime_visitor : public boost::static_visitor<output_variant>
	{
		runtime_map &map;
		client_db &db;
		const std::vector<boost::asio::ip::tcp::endpoint>& root_endpoint;

		runtime_visitor(runtime_map& map_, 
						client_db &db,
						const std::vector<boost::asio::ip::tcp::endpoint>& root_endpoint
						) :
			map(map_), db(db), root_endpoint(root_endpoint)
		{}

		output_variant operator() (const network::request_name& r) const
		{
			network::request_name_answer res_msg;
			res_msg.name = r.name;

			if (r.name == HYPERRUNTIME_NAME) {
				res_msg.success = true;
				res_msg.endpoints = root_endpoint;
			} else {
				runtime_map::iterator it = map.find(r.name);

				res_msg.success = (it != map.end());
				if (it != map.end()) 
					res_msg.endpoints = it->second.addr.tcp_endpoints;
			}

			return res_msg;
		}

		output_variant operator() (const network::register_name& r) const
		{
			using namespace boost::asio;

			ability_context ctx;
			ctx.addr.tcp_endpoints = r.endpoints;
			ctx.timeout_occurence = 0;

			runtime_map::iterator it = map.find(r.name);
			bool already_here = (it != map.end());
			if (!already_here) {
				map.insert(std::make_pair(r.name, ctx));
			}

			network::register_name_answer res_msg;
			res_msg.name = r.name;
			res_msg.success = !already_here;

			if (!already_here) {
				boost::shared_ptr<network::inform_new_agent> ptr_msg;
				ptr_msg = boost::make_shared<network::inform_new_agent>();
				ptr_msg->new_agents.push_back(r.name);
				std::for_each(map.begin(), map.end(), broadcast_inform(ptr_msg, db));
			}

			return res_msg;
		}

		output_variant operator() (const network::ping& p) const
		{
			runtime_map::iterator it = map.find(p.name);
			if (it != map.end())
				it->second.timeout_occurence = 0;

			return boost::mpl::void_();
		}

		struct agent_name
		{
			std::string operator() (const std::pair<std::string, ability_context>& p) const
			{
				return p.first;
			}
		};

		output_variant operator() (const network::request_list_agents& p) const
		{
			boost::shared_ptr<network::list_agents> agents = boost::make_shared<network::list_agents>();
			/* copy the request key in the answer */
			agents->id = p.id;
			agents->src = p.src;

			std::transform(map.begin(), map.end(), std::back_inserter(agents->all_agents),
						   agent_name());

			db[p.src].async_write(*agents,
					boost::bind(&handle_list_agents, boost::asio::placeholders::error, agents));
			return boost::mpl::void_();
		}
	};

	struct periodic_check
	{
		boost::asio::io_service& io_s_;

		boost::posix_time::time_duration delay_;
		boost::asio::deadline_timer timer_;

		runtime_map& map_;
		client_db& db_;

		periodic_check(boost::asio::io_service& io_s, 
					   boost::posix_time::time_duration delay,
					   runtime_map& map, client_db& db) :
			io_s_(io_s), delay_(delay), timer_(io_s_),
			map_(map), db_(db)
		{}


		void handle_write(const boost::system::error_code&, 
						  boost::shared_ptr<network::inform_death_agent> ptr)
		{
			// do nothing, decrement ptr ref, so it will be released correctly
			// when all write are finished
			(void) ptr;
		}

		struct inform_survivor {
			periodic_check & check;
			boost::shared_ptr<network::inform_death_agent> ptr;

			inform_survivor(periodic_check& check_,
					boost::shared_ptr<network::inform_death_agent> ptr_) :
				check(check_), ptr(ptr_) {}

			void operator() (const std::pair<std::string, ability_context>& p) const
			{
				check.db_[p.first].async_write(*ptr, 
						boost::bind(&periodic_check::handle_write, &check, 
									boost::asio::placeholders::error,
									ptr));
			}
		};

		void handle_timeout(const boost::system::error_code& e)
		{
			if (!e) {
				std::vector<std::string> dead_agents;
				runtime_map::iterator it = map_.begin();
				while (it != map_.end())
				{
					if (it->second.timeout_occurence > 2) {
						dead_agents.push_back(it->first);
						db_[it->first].close();
						map_.erase(it++);
					} else {
						it->second.timeout_occurence++;
						++it;
					}
				}

				if (! dead_agents.empty()) {
					boost::shared_ptr<network::inform_death_agent> ptr_msg;
					ptr_msg = boost::make_shared<network::inform_death_agent>();

					std::cout << "the following agents seems dead : ";
					std::copy(dead_agents.begin(), dead_agents.end(), 
							  std::ostream_iterator<std::string>(std::cout, " "));
					std::cout << std::endl;

					std::swap(ptr_msg->dead_agents, dead_agents);
					std::for_each(map_.begin(), map_.end(), inform_survivor(*this, ptr_msg));
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
	std::vector<boost::asio::ip::tcp::endpoint> root_endpoints;
	details::runtime_map map;
	details::runtime_actor actor(map);
	details::client_db db(actor);

	details::runtime_visitor runtime_vis(map, db, root_endpoints);

	typedef network::tcp::server<details::input_msg, 
								  details::output_msg, 
								  details::runtime_visitor
								> tcp_runtime_impl;
	tcp_runtime_impl runtime(4242, runtime_vis, actor.io_s);
	root_endpoints = runtime.local_endpoints();

	details::periodic_check check( actor.io_s, 
								   boost::posix_time::milliseconds(AGENT_TIMEOUT), 
								   map, db);
						 
	check.run();
	actor.io_s.run();
}
