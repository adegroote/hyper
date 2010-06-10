#ifndef _MODEL_ABILITY_HH_
#define _MODEL_ABILITY_HH_

#include <network/server_tcp_impl.hh>
#include <network/nameserver.hh>


namespace hyper {
	namespace model {

		namespace details {
			typedef boost::mpl::vector<> input_msg;
			typedef boost::mpl::vector<> output_msg;

			typedef boost::make_variant_over<input_msg>::type input_variant;
			typedef boost::make_variant_over<output_msg>::type output_variant;

			struct ability_visitor : public boost::static_visitor<output_variant>
			{
			};
		};

		struct ability {
			typedef network::tcp::server<details::input_msg, 
										 details::output_msg, 
										 details::ability_visitor>
			tcp_ability_impl;

			boost::asio::io_service io_s;
			network::name_client_sync name_client;

			ability(const std::string& name) : 
				name_client(io_s, "localhost", "4242")
			{
				std::pair<bool, boost::asio::ip::tcp::endpoint> p;
				p = name_client.register_name(name);
				if (p.first == true)
					std::cout << "Succesfully get an addr" << p.second << std::endl;
				else
					std::cout << "failed to get an addr" << std::endl;
			};

			void run()
			{
				io_s.run();
			}

			void stop()
			{
			}
		};
	};
};

#endif /* _MODEL_ABILITY_HH_ */
