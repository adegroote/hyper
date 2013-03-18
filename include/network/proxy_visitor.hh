
#ifndef _HYPER_NETWORK_PROXY_VISITOR_HH_
#define _HYPER_NETWORK_PROXY_VISITOR_HH_

#include <network/client_tcp_impl.hh>
#include <network/nameserver.hh>
#include <network/proxy_container.hh>
#include <network/utils.hh>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/void.hpp>
#include <boost/optional/optional.hpp>

namespace hyper {
	namespace network {
		typedef boost::mpl::vector< request_variable_value > proxy_input_msg;
		typedef boost::mpl::vector< boost::mpl::void_ > proxy_output_msg;
		typedef boost::make_variant_over< proxy_input_msg>::type proxy_input_variant;
		typedef boost::make_variant_over< proxy_output_msg>::type proxy_output_variant;
		
		template <typename Actor>
		struct proxy_visitor : public boost::static_visitor<proxy_output_variant>
		{
			Actor &actor;
			proxy_serializer& s;

			proxy_visitor(Actor& actor_, proxy_serializer& s_) : 
				actor(actor_), s(s_) {};

			void handle_proxy_output(const boost::system::error_code& e, 
					variable_value* v) const
			{
				(void) e;
				delete v;
			}

			proxy_output_variant operator() (const request_variable_value& r) const
			{
				boost::optional<std::string> p = s.eval(r.var_name);

				variable_value* res(new variable_value);
				res->id = r.id;
				res->src = r.src;
				res->var_name = r.var_name;
				res->success = p;
				if (p)
					res->value = *p;
				
				actor.client_db[r.src].async_write(*res, 
						boost::bind(&proxy_visitor::handle_proxy_output,
									this, boost::asio::placeholders::error,
									res));

				return boost::mpl::void_();
			}
		};
	}
}

#endif /* _HYPER_NETWORK_PROXY_VISITOR_HH_ */
