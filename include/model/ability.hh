#ifndef _MODEL_ABILITY_HH_
#define _MODEL_ABILITY_HH_

#include <network/actor_protocol.hh>
#include <network/log.hh>
#include <network/msg.hh>
#include <network/proxy.hh>
#include <network/ping.hh>
#include <network/nameserver.hh>
#include <network/server_tcp_impl.hh>

#include <model/execute.hh>
#include <model/logic_layer.hh>

#include <boost/make_shared.hpp>


namespace hyper {
	namespace model {

		struct ability;

		namespace details {

			typedef boost::mpl::vector<network::request_variable_value,
									   network::request_constraint,
									   network::variable_value,
									   network::request_constraint_answer> input_msg;
			typedef boost::mpl::vector<network::variable_value,
									   network::request_constraint_ack,
									   network::request_constraint_answer,
									   boost::mpl::void_> output_msg;

			typedef boost::mpl::vector<network::variable_value,
									   network::request_constraint_answer> input_cb;

			typedef boost::make_variant_over<input_msg>::type input_variant;
			typedef boost::make_variant_over<output_msg>::type output_variant;

			struct ability_visitor : public boost::static_visitor<output_variant>
			{
				ability &a;
				mutable size_t constraint_id;

				ability_visitor(ability& a_) : a(a_) {}

				output_variant operator() (const network::request_variable_value& m) const;
				output_variant operator() (const network::request_constraint& r) const;
				output_variant operator() (const network::variable_value & v) const;
				output_variant operator() (const network::request_constraint_answer& v) const;
			};
		}

		struct ability {
			typedef network::tcp::server<details::input_msg, 
										 details::output_msg, 
										 details::ability_visitor>
			tcp_ability_impl;

			typedef network::callback_database<details::input_cb>
				cb_db;


			boost::asio::io_service io_s;

			/* Lock for the ability context */
			boost::shared_mutex mtx;

			network::name_client name_client;

			/* Logger for the system */
			network::logger<network::name_client> logger;

			/* db to store expected callback */
			cb_db db;
			network::actor_client_database<ability> client_db;
			network::identifier base_id;

			network::proxy_serializer serializer;
			network::local_proxy proxy;
			network::actor::proxy_visitor<ability> proxy_vis;
			network::actor_protocol_visitor<ability> actor_vis;
			details::ability_visitor vis;
			std::string name;
			model::functions_map f_map;
			network::ping_process ping;

			boost::shared_ptr<tcp_ability_impl> serv;



			/* XXX
			 * logic_layer ctor use f_map, so initialize it after it 
			 *
			 * I'm not sure f_map and proxy must be exposed directly in the
			 * ability, as there are only useful to compute a remote
			 * constraint, which is done only in one place, in the logic_layer
			 */
			model::logic_layer logic;


			ability(const std::string& name_, int level = 0) : 
				name_client(io_s, "localhost", "4242"),
				logger(io_s, name_, "logger", name_client, level),
				client_db(*this),
				base_id(0),
				proxy_vis(*this, serializer),
				actor_vis(*this),
				vis(*this),
				name(name_),
				ping(io_s, boost::posix_time::milliseconds(100), name, "localhost", "4242"),
				logic(*this)
			{
				std::pair<bool, boost::asio::ip::tcp::endpoint> p;
				p = name_client.register_name(name);
				if (p.first == true) {
					std::cout << "Succesfully get an addr " << p.second << std::endl;
					serv = boost::shared_ptr<tcp_ability_impl> 
						(new tcp_ability_impl(p.second, vis, io_s));
				}
				else
					std::cout << "failed to get an addr" << std::endl;
			};

			template <typename T>
			void export_variable(const std::string& name, const T& value)
			{
				serializer.register_variable(name, value);
				proxy.register_variable(name, value);
			}

			void run()
			{
#ifndef HYPER_UNIT_TEST
				ping.run();
#endif
				io_s.run();
			}

			network::identifier gen_identifier()
			{
				return base_id++;
			}

			void stop()
			{
				ping.stop();
				serv->stop();
			}

			virtual ~ability() {}
		};
	}
}

#endif /* _MODEL_ABILITY_HH_ */
