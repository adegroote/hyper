#ifndef _MODEL_ABILITY_HH_
#define _MODEL_ABILITY_HH_

#include <network/proxy_container.hh>

#include <model/discover_root.hh>
#include <model/execute.hh>
#include <model/setter.hh>
#include <model/update.hh>

namespace hyper {
	namespace model {

		struct ability_impl;
		struct actor_impl;
		struct logic_layer;

		struct ability : public boost::noncopyable {


			boost::asio::io_service io_s;

			model::discover_root discover;
			actor_impl* actor;

			model::updater updater;
			network::proxy_serializer serializer;
			network::local_proxy proxy;
			model::setter setter;

			std::string name;
			model::functions_map f_map;

			/* our knoweldge about alive agents */
			std::set<std::string> alive_agents;

			ability_impl* impl;

			ability(const std::string& name_, int level = 0);

			template <typename T>
			void export_variable(const std::string& name, T& value);

			template <typename T>
			void export_writable_variable(const std::string& name, T& value);

			template <typename T>
			void export_variable(const std::string& name, T& value, 
								 const std::string& update);

			void test_run();
			void run();

			void stop();

			std::ostream& logger(int level);

			logic_layer& logic();

			virtual ~ability();

			protected:
			void start();

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
