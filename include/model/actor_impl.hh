#ifndef HYPER_MODEL_ACTOR_IMPL_HH_
#define HYPER_MODEL_ACTOR_IMPL_HH_

#include <network/actor_protocol.hh>
#include <network/log.hh>
#include <network/msg.hh>
#include <network/proxy.hh>
#include <network/nameserver.hh>

namespace hyper {
	namespace model {
		namespace details {
			typedef boost::mpl::vector<network::variable_value,
									   network::request_constraint_answer,
									   network::list_agents> input_cb;
		}
		
		struct discover_root;

		struct actor_impl {
			typedef network::callback_database<details::input_cb>
				cb_db;

			boost::asio::io_service& io_s;
			std::string name;

			network::name_client name_client;

			/* db to store expected callback */
			cb_db db;
			network::actor_client_database<actor_impl> client_db;
			network::identifier base_id;

			network::identifier gen_identifier()
			{
				return base_id++;
			}

			/* Logger for the system */
			network::logger<network::name_client> logger_;

			std::ostream& logger(int level)
			{
				return logger_(level);
			}

			actor_impl(boost::asio::io_service& io_s, const std::string& name,
					   int level, const discover_root& discover);
		};
	}
}

#endif /* HYPER_MODEL_ACTOR_IMPL_HH_ */
