
#ifndef _HYPER_NETWORK_PROXY_HH_
#define _HYPER_NETWORK_PROXY_HH_

#include <network/client_tcp_impl.hh>
#include <network/nameserver.hh>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/function.hpp>
#include <boost/optional/optional.hpp>
#include <boost/none.hpp>

#include <cassert>

namespace hyper {
	namespace network {

		template <typename T>
		struct proxy
		{
			T& value_;

			proxy(T& value) : value_(value) {};

			const T& operator() () const { return value_;}
		};

		template <typename T>
		std::string serialize_value(const T& value)
		{
			std::ostringstream archive_stream;
			boost::archive::text_oarchive archive(archive_stream);
			archive << value;
			return archive_stream.str();
		}

		template <typename T>
		boost::optional<T> deserialize_value(const std::string& value)
		{
			T res;
			try {
				std::istringstream archive_stream(value);
				boost::archive::text_iarchive archive(archive_stream);
				archive >> res;
				return res;
			} catch (std::exception& e) {
				return boost::optional<T>();
			}
		}

		class proxy_serializer
		{
			typedef std::map<std::string, boost::function <std::string ()> > serializer;
			serializer s;
			mutable boost::shared_mutex m_;

			public:
				proxy_serializer() {};
				template <typename T>
				bool register_variable(const std::string& name, const T& value)
				{
					std::string(*f) (const T& value) = &serialize_value<T>;
					std::pair< serializer::iterator, bool > p;

					boost::upgrade_lock<boost::shared_mutex> lock(m_);
					p = s.insert(std::make_pair(name, 
								boost::bind(f, boost::cref(value))));
					return p.second;
				}

				std::pair<bool, std::string> eval(const std::string& name) const
				{
					serializer::const_iterator it;
					boost::shared_lock<boost::shared_mutex> lock(m_);
					it = s.find(name);
					if (it == s.end()) 
						return std::make_pair(false, "");
					return std::make_pair(true, it->second());
				}
		};

		typedef boost::mpl::vector< request_variable_value > proxy_input_msg;
		typedef boost::mpl::vector< variable_value > proxy_output_msg;
		typedef boost::make_variant_over< proxy_input_msg>::type proxy_input_variant;
		typedef boost::make_variant_over< proxy_output_msg>::type proxy_output_variant;
	
		struct proxy_visitor : public boost::static_visitor<proxy_output_variant>
		{
			proxy_serializer& s;
	
			proxy_visitor(proxy_serializer& s_) : s(s_) {};
			
			proxy_output_variant operator() (const request_variable_value& r) const
			{
				std::pair<bool, std::string> p = s.eval(r.var_name);
				
				variable_value res;
				res.var_name = r.var_name;
				res.success = p.first;
				res.value = p.second;
	
				return res;
			}
		};


		template <typename T>
		class proxy_async_client {
			private:
				typedef tcp::client<proxy_output_msg> proxy_client;

				proxy_client c_;
				variable_value value_msg;

				template <typename Handler>
				void handle_value_request(const boost::system::error_code &e,
										  boost::optional<T>& value,
										  boost::tuple<Handler> handler)
				{
					if (e || value_msg.success == false ) 
						value = boost::none;
					else
						value = deserialize_value<T>(value_msg.value);

					boost::get<0>(handler)(e);
				}

			public:
				proxy_async_client(boost::asio::io_service& io_s) : c_(io_s) {}

				template <typename Handler>
				void async_connect(const boost::asio::ip::tcp::endpoint& endpoint,
										Handler handler)
				{
					return c_.async_connect(endpoint, handler);
				}

				void close() { c_.close(); }

				template <typename Handler>
				void async_request(const std::string& name, boost::optional<T>& value,
								  Handler handler) 
				{
					request_variable_value m;
					m.var_name = name;
					void (proxy_async_client::*f)(const boost::system::error_code &,
											      boost::optional<T>&,
												  boost::tuple<Handler>)
						= &proxy_async_client::template handle_value_request<Handler>;

					c_.async_request(m, value_msg, 
							boost::bind(f, this, 
											  _1, 
											 boost::ref(value), 
											 boost::make_tuple(handler)));
				}
		};

		/*
		 * In the remote case, @T must be serializable. In the remote case
		 * proxy, the proxy object is a consummer of a remote publised
		 * information. 
		 */
		template <typename T, typename Resolver>
		class remote_proxy
		{
			private:
				boost::optional<T> value_;
				std::string ability_name_;
				std::string var_name_, var_value_;

				proxy_async_client<T> c;
				Resolver& r_;
				boost::asio::ip::tcp::endpoint endpoint_;

			private:
				template <typename Handler>
				void handle_request(const boost::system::error_code &e,
									boost::tuple<Handler> handler)
				{
					boost::get<0>(handler)(e);
				}

				template <typename Handler>
				void handle_connect(const boost::system::error_code& e,
									boost::tuple<Handler> handler)
				{
					if (e) 
						boost::get<0>(handler)(e);
					else {
						void (remote_proxy::*f) (const boost::system::error_code&,
								boost::tuple<Handler>) =
							&remote_proxy::template handle_request<Handler>;

						c.async_request(var_name_, value_,
							boost::bind(f, this, boost::asio::placeholders::error, handler));
					}
				}

				template <typename Handler>
				void handle_resolve(const boost::system::error_code& e,
									boost::tuple<Handler> handler)
				{
					if (e) 
						boost::get<0>(handler)(e);
					else {
						void (remote_proxy::*f) (const boost::system::error_code&,
								boost::tuple<Handler>) =
							&remote_proxy::template handle_connect<Handler>;

						c.async_connect(endpoint_, 
							boost::bind(f, this, boost::asio::placeholders::error, handler));
					}
				}

			public:
				remote_proxy(boost::asio::io_service & io_s,
							 const std::string& ability_name,
							 const std::string& var_name,
							 Resolver& r) :
					ability_name_(ability_name), var_name_(var_name), c(io_s), r_(r) {}

				/* Asynchronously make an request to remote ability
				 * On completion (in the @Handler call), it is safe to call
				 * operator() to get the value of the remote  variable.
				 */
				template <typename Handler>
				void async_get(Handler handler) 
				{ 
					void (remote_proxy::*f) (const boost::system::error_code&,
							boost::tuple<Handler>) =
						&remote_proxy::template handle_resolve<Handler>;

					r_.async_resolve(ability_name_, endpoint_,
							boost::bind(f, this,
										 boost::asio::placeholders::error,
										 boost::make_tuple(handler)));
				}

				const boost::optional<T>& operator() () const { return value_; };
		};


	}
}

#endif /* _HYPER_NETWORK_PROXY_HH_ */
