
#ifndef _HYPER_NETWORK_PROXY_HH_
#define _HYPER_NETWORK_PROXY_HH_

#include <network/client_tcp_impl.hh>
#include <network/nameserver.hh>
#include <network/utils.hh>

#include <boost/any.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/optional/optional.hpp>
#include <boost/none.hpp>
#include <boost/thread/locks.hpp>

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
			} catch (std::exception& ) {
				return boost::optional<T>();
			}
		}

		template <typename T>
		boost::any capture_value(const T& value)
		{
			return value;
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

		class local_proxy 
		{
			typedef std::map<std::string, boost::function<boost::any ()> > l_proxy;
			l_proxy l;
			mutable boost::shared_mutex m_;

			public:
				local_proxy() {};
				template <typename T>
				bool register_variable(const std::string& name, const T& value)
				{
					boost::any(*f) (const T& value) = &capture_value<T>;
					std::pair< l_proxy::iterator, bool > p;

					boost::upgrade_lock<boost::shared_mutex> lock(m_);
					p = l.insert(std::make_pair(name, 
								boost::bind(f, boost::cref(value))));
					return p.second;
				}

				/* 
				 * It will be nice to pass a boost::system::error to know the
				 * failure cause
				 */
				template <typename T>
				boost::optional<T> eval(const std::string& name) const
				{
					l_proxy::const_iterator it;
					boost::shared_lock<boost::shared_mutex> lock(m_);
					it = l.find(name);
					if (it == l.end()) 
						return boost::none;
					try { 
						T value = boost::any_cast<T>(it->second());
						return value;
					} catch (const boost::bad_any_cast &) {
						return boost::none;
					}

					return boost::none;
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
		class proxy_async_client 
		{
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
				void async_connect(const std::vector<boost::asio::ip::tcp::endpoint>& endpoint,
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
		 * Type erasure for remote_proxy 
		 */
		class remote_proxy_base 
		{
			public:
				typedef boost::function<void (const boost::system::error_code&)> callback_fun;
				virtual void async_get_result(callback_fun) = 0;
				virtual boost::any extract_result() const = 0;
				virtual ~remote_proxy_base() {}
		};

		/*
		 * In the remote case, @T must be serializable. In the remote case
		 * proxy, the proxy object is a consummer of a remote publised
		 * information. 
		 */
		template <typename T, typename Resolver>
		class remote_proxy : public remote_proxy_base
		{
			public:
				typedef boost::optional<T> type;

			private:
				type value_;
				std::string ability_name_;
				std::string var_name_, var_value_;
				bool finished;
				bool connected;

				proxy_async_client<T> c;
				Resolver& r_;
				name_resolve solver;

			private:

				template <typename Handler>
				void send_request(boost::tuple<Handler> handler)
				{
					void (remote_proxy::*f) (const boost::system::error_code&,
							boost::tuple<Handler>) =
						&remote_proxy::template handle_request<Handler>;

					c.async_request(var_name_, value_,
							boost::bind(f, this,
										   boost::asio::placeholders::error, handler));
				}

				template <typename Handler>
				void handle_request(const boost::system::error_code &e,
									boost::tuple<Handler> handler)
				{
					finished = true;
					boost::get<0>(handler)(e);
				}

				template <typename Handler>
				void handle_connect(const boost::system::error_code& e,
									boost::tuple<Handler> handler)
				{
					if (e) 
						boost::get<0>(handler)(e);
					else {
						connected=true;
						send_request<Handler> (handler);
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

						c.async_connect(solver.endpoints(), 
							boost::bind(f, this,
										boost::asio::placeholders::error, handler));
					}
				}

				template <typename Handler>
				void async_get_(boost::tuple<Handler> handler)
				{
					finished = false;
					value_ = boost::none;

					if (!connected) {
					void (remote_proxy::*f) (const boost::system::error_code&,
							boost::tuple<Handler>) =
						&remote_proxy::template handle_resolve<Handler>;
					solver.name(ability_name_);
					
					r_.async_resolve(solver,
							boost::bind(f, this,
										 boost::asio::placeholders::error,
										 handler));
					} else {
						send_request<Handler> (handler);
					}
				}

			public:
				remote_proxy(boost::asio::io_service & io_s,
							 const std::string& ability_name,
							 const std::string& var_name,
							 Resolver& r) :
					ability_name_(ability_name), var_name_(var_name), 
					finished(false), connected(false),
					c(io_s), r_(r) {}

				/* Asynchronously make an request to remote ability
				 * On completion (in the @Handler call), it is safe to call
				 * operator() to get the value of the remote  variable.
				 */
				template <typename Handler>
				void async_get(Handler handler) 
				{ 
					return async_get_(boost::make_tuple(handler));
				}

				void async_get_result(callback_fun f)
				{
					return async_get_(boost::make_tuple(f));
				}

				void abort() { c.close(); };

				const type& operator() () const { return value_; };

				boost::any extract_result() const { return value_; };

				bool is_finished() const { return finished; };
		};

		template <typename T, typename Resolver>
		class remote_proxy_sync
		{
			public:
			remote_proxy_sync(
							 boost::asio::io_service& io_service,
							 const std::string& ability_name,
							 const std::string& var_name,
							 Resolver& r):
				io_service_(io_service),
				deadline_(io_service_),
				client(io_service_, ability_name, var_name, r) 
			{
				deadline_.expires_at(boost::posix_time::pos_infin);

				// Start the persistent actor that checks for deadline expiry.
				check_deadline();
			}

			const boost::optional<T>& get(boost::posix_time::time_duration timeout)
			{
				deadline_.expires_from_now(timeout);
				boost::system::error_code ec = boost::asio::error::would_block;

				client.async_get(boost::bind(&remote_proxy_sync::handle_answer, 
											 this,
											 boost::asio::placeholders::error,
											 boost::ref(ec))
								 );

				// Block until the asynchronous operation has completed.:D
				do io_service_.run_one(); while (ec == boost::asio::error::would_block);

				deadline_.expires_at(boost::posix_time::pos_infin);

				return client();
			}

			private:
			  void check_deadline()
			  {
				  // Check whether the deadline has passed. We compare the deadline against
				  // the current time since a new asynchronous operation may have moved the
				  // deadline before this actor had a chance to run.
				  if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
				  {
					  client.abort();
					  deadline_.expires_at(boost::posix_time::pos_infin);
				  }
					
				// Put the actor back to sleep.
				deadline_.async_wait(boost::bind(&remote_proxy_sync::check_deadline, this));
			}

			  void handle_answer(const boost::system::error_code& e,
								 boost::system::error_code& wait_e)
			  {
				  wait_e = e;
			  }

			  boost::asio::io_service& io_service_;
			  boost::asio::deadline_timer deadline_;
			  remote_proxy<T, Resolver> client;
		};

		namespace details {
			template <typename T, typename Resolver>
			struct to_proxy 
			{
				typedef boost::shared_ptr<remote_proxy<T, Resolver> > type;
			};

			template <typename T>
			struct to_internal_type
			{
				typedef const typename boost::optional<T>& type;
			};

			template <typename tupleT, typename vectorT, typename Resolver>
			struct init_proxys
			{
				tupleT & t;
				boost::asio::io_service& io_s;
				enum { size = boost::mpl::size<vectorT>::type::value };
				const boost::array<std::pair<std::string, std::string>, size>& vars;
				Resolver& r;

				init_proxys(tupleT& t_, 
							boost::asio::io_service& io_s_,
							const boost::array<std::pair<std::string, std::string>, size>& vars_,
							Resolver& r_):
					t(t_), io_s(io_s_), vars(vars_), r(r_) {}

				template <typename U>
				void operator() (U unused)
				{
					(void) unused;
					typedef remote_proxy<typename boost::mpl::at<vectorT, U>::type, Resolver> proxy_;

					boost::get<U::value>(t) = boost::shared_ptr<proxy_>(
						new proxy_ (io_s, vars[U::value].first, 
										  vars[U::value].second, r)
					);
				}
			};

			template <typename tupleT>
			struct abort_proxys 
			{
				tupleT & t;

				abort_proxys(tupleT& t_): t(t_) {};

				template <typename U> // U models mpl::int_
				void operator() (U unused)
				{
					(void) unused;
					if (!boost::get<U::value>(t)->is_finished())
						boost::get<U::value>(t)->abort();
				}
			};

			template <typename tupleT>
			struct proxy_finished
			{
				const tupleT & t;
				bool & res;

				proxy_finished(const tupleT& t_, bool& b): t(t_), res(b) {};

				template <typename U> // U models mpl::int_
				void operator() (U unused)
				{
					(void) unused;
					res = res &&  (boost::get<U::value>(t)->is_finished());
				}
			};

			template <typename tupleT>
			struct proxy_successed
			{
				const tupleT & t;
				bool & res;

				proxy_successed(const tupleT& t_, bool& b): t(t_), res(b) {};

				template <typename U> // U models mpl::int_
				void operator() (U unused)
				{
					(void) unused;
					res = res && (*(boost::get<U::value>(t)))();
				}
			};

			template <typename tupleT, typename Handler, typename vectorT, typename Resolver>
			struct call_proxy;
		}



		template <typename vectorT, typename Resolver>
		struct remote_proxy_n 
		{
			typedef typename boost::mpl::transform<
				vectorT, 
				details::to_proxy<boost::mpl::_1, Resolver> 
			>::type seq;

			typedef typename boost::mpl::transform<
				vectorT,
				details::to_internal_type<boost::mpl::_1>
			>::type seqReturn;

			typedef typename to_tuple<seq>::type tuple_proxy;
			enum { size = boost::mpl::size<vectorT>::type::value };
			typedef boost::mpl::range_c<size_t, 0, size> range;


			/*
			 * proxy is a tuple of boost::shared_ptr< remote_proxy <> > We need
			 * to use some ptr because we can't initialize it in the
			 * initialisation list of remote_proxy_n.
			 */
			tuple_proxy proxys;

			remote_proxy_n(boost::asio::io_service& io_s,
						   const boost::array<std::pair<std::string, std::string>, size>&  vars,
						   Resolver& r) 
			{
				details::init_proxys<tuple_proxy, vectorT, Resolver> 
					init_(proxys, io_s, vars, r);
				boost::mpl::for_each<range> (init_);
			};

			~remote_proxy_n() 
			{
				abort();
			}

			template <typename Handler>
			void handle_request(const boost::system::error_code& e,
								boost::tuple<Handler> handler) 
			{
				if (e) {
					/* Discard cancelled error */
					if (e == boost::asio::error::operation_aborted)
						return;

					abort();

					boost::get<0>(handler)(e);
				} else {
					/* check if everything has terminated. If it is ok, call
					 * handler(e). Otherwise, do nothing 
					 */
					bool is_finished = true;
					details::proxy_finished<tuple_proxy> finish_(proxys, is_finished);
					boost::mpl::for_each<range> (finish_);

					if (is_finished) 
						boost::get<0>(handler)(e);
				}
			}
			/*
			 * async_get will ask all the remote ability to get the value of
			 * their respective variable, under a certain timeout.
			 *
			 * When an ability answers :
			 *		- if it is a failure ( operator() return a boost::none ),
			 *		we abort other requests, as we can't compute the whole
			 *		expression
			 *		- if it is a success, but we are still waiting some other
			 *		answer, don't do anything
			 *		- if all abilities have answered, handler is called and you
			 *		can call at_c<i> on each element of the tuple
			 */
			template <typename Handler>
			void async_get(Handler handler)
			{
				void (remote_proxy_n::*f)(const boost::system::error_code&,
										  boost::tuple<Handler> handler) =
					&remote_proxy_n::template handle_request<Handler>;

				details::call_proxy<
					tuple_proxy, 
					boost::function<void (const boost::system::error_code&)>,
					vectorT, 
					Resolver
				> call_(proxys, boost::bind(f, this, 
							boost::asio::placeholders::error,
							boost::make_tuple(handler)),
						*this);;
				boost::mpl::for_each<range> (call_); 
			}

			void abort()
			{
				/* abort other request */
				details::abort_proxys<tuple_proxy> abort_(proxys);
				boost::mpl::for_each<range> (abort_);
			}

			bool is_successful() const
			{
				bool is_success = true;
				details::proxy_successed<tuple_proxy> success_(proxys, is_success);
				boost::mpl::for_each<range> (success_);
				return is_success;
			}

			template <size_t i>
			typename boost::mpl::at<seqReturn, boost::mpl::int_<i> >::type at_c() const
			{
				return (*(boost::get<i>(proxys)))();
			}

		};

		namespace details {
			template <typename tupleT, typename Handler, typename vectorT, typename Resolver>
			struct call_proxy
			{
				tupleT& t;
				Handler handler;
				remote_proxy_n<vectorT, Resolver>& r_proxy;

				call_proxy(tupleT& t_, Handler handler_, 
						   remote_proxy_n<vectorT, Resolver>& r_proxy_): 
					t(t_), handler(handler_), r_proxy(r_proxy_) {};

				template <typename U>
				void operator()(U unused)
				{
					(void) unused;
					boost::get<U::value>(t)->async_get(handler);
				}
			};
		}

	
		namespace actor {
			

			typedef boost::mpl::vector< request_variable_value > proxy_input_msg;
			typedef boost::mpl::vector< boost::mpl::void_ > proxy_output_msg;
			typedef boost::make_variant_over< proxy_input_msg>::type proxy_input_variant;
			typedef boost::make_variant_over< proxy_output_msg>::type proxy_output_variant;
			
			template <typename Actor>
			struct proxy_visitor : public boost::static_visitor<proxy_output_variant>
			{
				proxy_serializer& s;
				Actor &actor;

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
					std::pair<bool, std::string> p = s.eval(r.var_name);

					variable_value* res(new variable_value);
					res->id = r.id;
					res->src = r.src;
					res->var_name = r.var_name;
					res->success = p.first;
					res->value = p.second;
					
					actor.client_db[r.src].async_write(*res, 
							boost::bind(&proxy_visitor::handle_proxy_output,
									    this, boost::asio::placeholders::error,
										res));

					return boost::mpl::void_();
				}
			};

			template <typename T>
			struct remote_value
			{
				std::string src;
				request_variable_value msg;
				variable_value ans;

				boost::optional<T> value;
				bool terminated;
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

					typedef typename to_tuple<seq>::type type;
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

				template <typename Actor, typename tupleT, typename Handler>
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
					bool res;
					details::remote_values_is_terminated<tupleT> is_terminated_(values, res);
					boost::mpl::for_each<range> (is_terminated_);
					return res;
				}

				template <size_t i>
				typename boost::mpl::at<seqReturn, boost::mpl::int_<i> >::type at_c() const
				{
					return (boost::get<i>(values)).value;
				}
			};

			template <typename Actor>
			class remote_proxy 
			{
				private:
					request_variable_value msg;
					variable_value ans;
					Actor & actor;

					template <typename T, typename Handler>
					void handle_get(const boost::system::error_code& e,
								    T& output,
									boost::tuple<Handler> handler)
					{
						if (e || ans.success == false ) 
							output = boost::none;
						else
							output = deserialize_value<typename T::value_type>(ans.value);

						boost::get<0>(handler)(e);
					}

					template <typename vectorT, typename Handler>
					void handle_remote_values_get(const boost::system::error_code& e,
									remote_values<vectorT>& values,
									boost::tuple<Handler> handler)
					{
						if (values.is_terminated())
							boost::get<0>(handler)(e);
					}

					template <typename T, typename Handler>
					void handle_remote_value_get(const boost::system::error_code& e,
									remote_value<T>& value,
									boost::tuple<Handler> handler)
					{
						value.terminated = true;

						if (e || value.ans.success == false ) 
							value.value = boost::none;
						else
							value.value = deserialize_value<T>(ans.value);

						boost::get<0>(handler)(e);
					}

				public:
					remote_proxy(Actor& actor_) : actor(actor_) {}
					
					template <typename T, typename Handler>
					void async_get(const std::string& actor_dst,
								   const std::string& var_name,
								   T & output,
								   Handler handler)
					{
						void (remote_proxy::*f)(const boost::system::error_code&,
								T& output,
								boost::tuple<Handler> handler) =
							&remote_proxy::template handle_get<T, Handler>;

						msg.var_name = var_name;
						actor.client_db[actor_dst].async_request(msg, ans,
								boost::bind(f, this,
										   boost::asio::placeholders::error,
										   boost::ref(output),
										   boost::make_tuple(handler)));
					}

					template <typename T, typename Handler>
					void async_get(remote_value<T>& value, Handler handler)
					{
						void (remote_proxy::*f)(const boost::system::error_code&,
								remote_value<T>& output,
								boost::tuple<Handler> handler) =
							&remote_proxy::template handle_remote_value_get<T, Handler>;

						actor.client_db[value.src].async_request(
								value.msg, value.ans, 
								boost::bind(f, this, 
											boost::asio::placeholders::error,
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

						details::remote_value_async_get<Actor, 
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
				template <typename Actor, typename tupleT, typename Handler>
				struct remote_value_async_get
				{
					remote_proxy<Actor>& proxy;
					tupleT& t;
					Handler handler;

					remote_value_async_get(remote_proxy<Actor>& proxy, tupleT& t, Handler handler) : 
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
}

#endif /* _HYPER_NETWORK_PROXY_HH_ */
