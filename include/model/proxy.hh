#ifndef _HYPER_MODEL_PROXY_HH_
#define _HYPER_MODEL_PROXY_HH_

#include <network/msg_proxy.hh>
#include <network/proxy_container.hh>
#include <network/utils.hh>
#include <network/log_level.hh>

#include <boost/asio/placeholders.hpp>
#include <boost/array.hpp>
#include <boost/function/function1.hpp>
#include <boost/function/function2.hpp>
#include <boost/make_shared.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/system/error_code.hpp>

namespace hyper {
	namespace model {
			struct ability;

			template <typename T>
			struct remote_value
			{
				std::string src;
				network::request_variable_value msg;
				network::variable_value ans;

				boost::optional<T> value;
				bool terminated;

				remote_value() {}
				remote_value(const std::string& src, const std::string& var_name) : 
					src(src), value(boost::none), terminated(false)
				{
					msg.var_name = var_name;
				}
			};

			namespace details {
				template <typename T>
				struct to_remote_value {
					typedef remote_value<T> type;
				};

				template <typename T>
				struct to_internal_type
				{
					typedef const typename boost::optional<T>& type;
				};

				template <typename vectorT>
				struct to_remote_tuple {
					typedef typename boost::mpl::transform<
								vectorT, 
								to_remote_value<boost::mpl::_1> 
							>::type seq;

					typedef typename network::to_tuple<seq>::type type;
				};

				template <typename tupleT, int size>
				struct init_remote_values
				{
					typedef boost::array<std::pair<std::string, std::string>,
										 size
										> remote_vars_conf;
					tupleT& t;
					const remote_vars_conf& vars;

					init_remote_values(tupleT& t,
									   const remote_vars_conf& vars) :
						t(t), vars(vars) {}

					template <typename U>
					void operator() (U)
					{
						boost::get<U::value>(t).src = vars[U::value].first;
						boost::get<U::value>(t).msg.var_name = vars[U::value].second;
						boost::get<U::value>(t).value = boost::none;
						boost::get<U::value>(t).terminated = false;
					}
				};

				template <typename tupleT>
				struct reset_remote_values
				{
					tupleT & t;

					reset_remote_values(tupleT& t_): t(t_) {};

					template <typename U> // U models mpl::int_
					void operator() (U) 
					{
						boost::get<U::value>(t).value = boost::none;
						boost::get<U::value>(t).terminated = false;
					}
				};

				template <typename tupleT>
				struct remote_values_is_terminated 
				{
					tupleT& t;
					bool &res;

					remote_values_is_terminated(tupleT& t, bool& res) :
						t(t), res(res) 
					{}

					template <typename U>
					void operator() (U)
					{
						res = res && boost::get<U::value>(t).terminated;
					}
				};

				template <typename tupleT>
				struct remote_values_is_valid
				{
					tupleT& t;
					bool &res;

					remote_values_is_valid(tupleT& t, bool& res) :
						t(t), res(res) 
					{}

					template <typename U>
					void operator() (U)
					{
						res = res && boost::get<U::value>(t).value;
					}
				};

				template <typename tupleT, typename Handler>
				struct remote_value_async_get;
			}

			template <typename vectorT>
			struct remote_values
			{
				typedef typename details::to_remote_tuple<vectorT>::type tupleT;
				enum { size = boost::mpl::size<vectorT>::type::value };
				typedef boost::mpl::range_c<size_t, 0, size> range;
				typedef boost::array<std::pair<std::string, std::string>,
									 size> remote_vars_conf;

				typedef typename boost::mpl::transform<
					vectorT,
					details::to_internal_type<boost::mpl::_1>
				>::type seqReturn;

				tupleT values;

				// XXX only valid if size == 0
				remote_values() {}

				remote_values(const remote_vars_conf& vars)
				{
					details::init_remote_values<tupleT, size> init_(values, vars);
					boost::mpl::for_each<range> (init_);
				}

				void reset()
				{
					details::reset_remote_values<tupleT> reset_(values);
					boost::mpl::for_each<range> (reset_);
				}

				bool is_terminated()
				{
					bool res = true;
					details::remote_values_is_terminated<tupleT> is_terminated_(values, res);
					boost::mpl::for_each<range> (is_terminated_);
					return res;
				}

				bool is_valid()
				{
					bool res = true;
					details::remote_values_is_valid<tupleT> is_valid_(values, res);
					boost::mpl::for_each<range> (is_valid_);
					return res;
				}

				template <size_t i>
				typename boost::mpl::at<seqReturn, boost::mpl::int_<i> >::type at_c() const
				{
					return (boost::get<i>(values)).value;
				}
			};

			class remote_proxy 
			{
				private:
					network::request_variable_value msg;
					network::variable_value ans;
					model::ability& a;

					typedef boost::function<void (const boost::system::error_code& e,
												  network::identifier id)> cb_fun;

					void async_ask(const std::string& dst, 
								   const network::request_variable_value& msg,
								   network::variable_value& ans,
								   cb_fun cb);

					void clean_up(network::identifier id);

					template <typename T, typename Handler>
					void handle_get(const boost::system::error_code& e,
									network::identifier id,
								    T& output,
									boost::tuple<Handler> handler)
					{
						if (e || ans.success == false ) {
							output = boost::none;
							a.logger(WARNING) << "Failed to get the value of " << ans.var_name << std::endl;
						} else {
							output = network::deserialize_value<typename T::value_type>(ans.value);
						}

						clean_up(id);
						boost::get<0>(handler)(e);
					}

					template <typename vectorT, typename Handler>
					void handle_remote_values_get(const boost::system::error_code& e,
									remote_values<vectorT>& values,
									boost::tuple<Handler> handler)
					{
						if (values.is_terminated()) {
							if (values.is_valid())
								boost::get<0>(handler)(e);
							else
								boost::get<0>(handler)(
									make_error_code(boost::system::errc::invalid_argument));
						}
					}

					template <typename T, typename Handler>
					void handle_remote_value_get(const boost::system::error_code& e,
									network::identifier id,
									remote_value<T>& value,
									boost::tuple<Handler> handler)
					{
						value.terminated = true;

						if (e || value.ans.success == false) {
							value.value = boost::none;
							a.logger(WARNING) << "Failed to get the value of " << value.ans.var_name << std::endl;
						} else { 
							value.value = network::deserialize_value<T>(value.ans.value);
						}

						clean_up(id);
						boost::get<0>(handler)(e);
					}

				public:
					remote_proxy(model::ability& a);
					
					template <typename T, typename Handler>
					void async_get(const std::string& actor_dst,
								   const std::string& var_name,
								   T & output,
								   Handler handler)
					{
						void (remote_proxy::*f)(const boost::system::error_code&,
								network::identifier,
								T& output,
								boost::tuple<Handler> handler) =
							&remote_proxy::template handle_get<T, Handler>;

						msg.var_name = var_name;
						async_ask(actor_dst, msg, ans, 
										boost::bind(f, this,
												    boost::asio::placeholders::error, _2,
													boost::ref(output),
													boost::make_tuple(handler)));
					}

					template <typename T, typename Handler>
					void async_get(remote_value<T>& value, Handler handler)
					{
						void (remote_proxy::*f)(const boost::system::error_code&,
								network::identifier,
								remote_value<T>& output,
								boost::tuple<Handler> handler) =
							&remote_proxy::template handle_remote_value_get<T, Handler>;

						async_ask(value.src, value.msg, value.ans, 
								boost::bind(f, this, 
											boost::asio::placeholders::error, _2,
											boost::ref(value),
											boost::make_tuple(handler)));
					}

					template <typename vectorT, typename Handler>
					void async_get(remote_values<vectorT>& values, Handler handler)
					{
						values.reset();
						void (remote_proxy::*f)(const boost::system::error_code&,
									remote_values<vectorT>& values, 
									boost::tuple<Handler> handler)
							= & remote_proxy::template handle_remote_values_get<vectorT, Handler>;

						details::remote_value_async_get<
											   typename remote_values<vectorT>::tupleT,
											   boost::function<void (const boost::system::error_code&)>
											   > async_get_(*this, values.values, 
									boost::bind(f, this, boost::asio::placeholders::error,
														 boost::ref(values),
														 boost::make_tuple(handler)));
						boost::mpl::for_each<typename remote_values<vectorT>::range> (async_get_);
					}
			};

			namespace details {
				template <typename tupleT, typename Handler>
				struct remote_value_async_get
				{
					remote_proxy& proxy;
					tupleT& t;
					Handler handler;

					remote_value_async_get(remote_proxy& proxy, tupleT& t, Handler handler) : 
						proxy(proxy), t(t), handler(handler)
					{}

					template <typename U>
					void operator() (U)
					{
						proxy.async_get(boost::get<U::value>(t), handler);
					}
				};
			}
	}
}

#endif /* _HYPER_MODEL_PROXY_HH_ */
