#ifndef _MODEL_ABILITY_HH_
#define _MODEL_ABILITY_HH_

#include <network/server_tcp_impl.hh>
#include <network/msg.hh>
#include <network/proxy.hh>
#include <network/ping.hh>
#include <network/nameserver.hh>

#include <model/execute.hh>

#include <boost/make_shared.hpp>


namespace hyper {
	namespace model {

		namespace details {
			typedef boost::mpl::vector<network::request_variable_value,
									   network::request_constraint>input_msg;
			typedef boost::mpl::vector<network::variable_value,
									   network::request_constraint_ack,
									   network::request_constraint_answer> output_msg;

			typedef boost::make_variant_over<input_msg>::type input_variant;
			typedef boost::make_variant_over<output_msg>::type output_variant;

			struct ability_visitor : public boost::static_visitor<output_variant>
			{
				boost::shared_mutex& mtx;
				network::proxy_visitor& proxy_vis;

				ability_visitor(boost::shared_mutex& mtx_, 
								network::proxy_visitor& proxy_vis_) : 
					mtx(mtx_), proxy_vis(proxy_vis_) {}

				output_variant operator() (const network::request_variable_value& m) const
				{
					boost::shared_lock<boost::shared_mutex> lock(mtx);
					return proxy_vis(m);
				}

				output_variant operator() (const network::request_constraint& r) const
				{
					std::cout << "receive constraint " << r.constraint << std::endl;
					network::request_constraint_ack ack;
					ack.acked = true;
					ack.id = 0;
					return ack;
				}
			};
		}

		struct ability {
			typedef network::tcp::server<details::input_msg, 
										 details::output_msg, 
										 details::ability_visitor>
			tcp_ability_impl;

			boost::asio::io_service io_s;

			/* Lock for the ability context */
			boost::shared_mutex mtx;

			network::name_client name_client;
			network::proxy_serializer serializer;
			network::local_proxy proxy;
			network::proxy_visitor proxy_vis;
			details::ability_visitor vis;
			model::functions_map f_map;
			std::string name;
			network::ping_process ping;

			boost::shared_ptr<tcp_ability_impl> serv;


			ability(const std::string& name_) : 
				name_client(io_s, "localhost", "4242"),
				proxy_vis(serializer),
				vis(mtx, proxy_vis),
				name(name_),
				ping(io_s, boost::posix_time::milliseconds(100), name, "localhost", "4242")
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

			void stop()
			{
				ping.stop();
				serv->stop();
			}
		};
	}
}

#endif /* _MODEL_ABILITY_HH_ */
