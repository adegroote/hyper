#ifndef _MODEL_ABILITY_HH_
#define _MODEL_ABILITY_HH_

#include <network/actor_protocol.hh>
#include <network/msg.hh>
#include <network/proxy.hh>
#include <network/nameserver.hh>

#include <model/discover_root.hh>
#include <model/execute.hh>
#include <model/update.hh>

#include <boost/scoped_ptr.hpp>


namespace hyper {
	namespace model {

		struct ability_impl;
		struct logic_layer;

		namespace details {
			typedef boost::mpl::vector<network::variable_value,
									   network::request_constraint_answer,
									   network::list_agents> input_cb;
		}

		struct ability : public boost::noncopyable {

			typedef network::callback_database<details::input_cb>
				cb_db;


			boost::asio::io_service io_s;

			/* Lock for the ability context */
			boost::shared_mutex mtx;

			model::discover_root discover;
			network::name_client name_client;

			/* db to store expected callback */
			cb_db db;
			network::actor_client_database<ability> client_db;
			network::identifier base_id;

			model::updater updater;
			network::proxy_serializer serializer;
			network::local_proxy proxy;
			std::string name;
			model::functions_map f_map;

			/* our knoweldge about alive agents */
			std::set<std::string> alive_agents;

			ability_impl* impl;

			ability(const std::string& name_, int level = 0);

			template <typename T>
			void export_variable(const std::string& name, T& value);

			template <typename T>
			void export_variable(const std::string& name, T& value, 
								 const std::string& update);

			void test_run();
			void run();

			network::identifier gen_identifier()
			{
				return base_id++;
			}

			void stop();

			std::ostream& logger(int level);

			logic_layer& logic();

			virtual ~ability();

			private:
			template <typename T>
			void export_variable_helper(const std::string& name, const T& value)
			{
				serializer.register_variable(name, value);
				proxy.register_variable(name, value);
			}

			typedef std::pair<network::request_list_agents, network::list_agents> get_list_agents;
			void handle_list_agents(const boost::system::error_code& e,
									network::identifier id,
									boost::shared_ptr<get_list_agents> ptr);
		};
	}
}

#endif /* _MODEL_ABILITY_HH_ */
