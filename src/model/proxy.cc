#include <model/actor_impl.hh>
#include <model/ability.hh>
#include <model/proxy.hh>

namespace hyper {
	namespace model {
		remote_proxy::remote_proxy(model::ability& a) : a(a) {}

		void remote_proxy::async_ask(const std::string& dst, const network::request_variable_value& msg,
							  network::variable_value& ans, cb_fun cb)
		{
			a.actor->client_db[dst].async_request(msg, ans, cb);
		}

		void remote_proxy::clean_up(network::identifier id)
		{
			a.actor->db.remove(id);
		}
	}
}
