#ifndef HYPER_MODEL_UPDATE_IMPL_HH
#define HYPER_MODEL_UPDATE_IMPL_HH

#include <network/runtime_error.hh>

#include <compiler/scope.hh>
#include <model/ability.hh>
#include <model/proxy.hh>
#include <model/update.hh>

namespace hyper {
	namespace model {

		namespace details {

			template <typename T>
			struct remote_args {
				T& value_to_bind;
				remote_proxy proxy;
				boost::optional<T> tmp;

				remote_args(model::ability& a, T& to_bind) :
					value_to_bind(to_bind), proxy(a) {}
			};

			template <typename T>
			void cb_proxy(const boost::system::error_code &e,
						  remote_args<T>* args,
						  updater::cb_type cb)
			{
				if (e) {
					delete args;
					return cb(e, hyper::network::empty_error_context); // XXX
				}

				if (args->tmp) {
					args->value_to_bind = *(args->tmp);
					delete args;
					return cb(boost::system::error_code(), hyper::network::empty_error_context); // XXX
				}

				delete args;
				// XXX need better error handling
				return cb(boost::asio::error::invalid_argument, hyper::network::empty_error_context); //XXX 
			}

			template <typename T>
			void call_proxy(ability &a, 
							const std::string& var, T& local_value, 
							network::identifier id, const std::string& src,
							updater::cb_type cb)
			{
				(void) id; (void) src;
				void (*f) (const boost::system::error_code&, 
						   remote_args<T>* args,
						   updater::cb_type cb) = cb_proxy<T>;

				remote_args<T>* args(new remote_args<T>(a, local_value));

				std::pair<std::string, std::string> p =
							compiler::scope::decompose(var);

				return args->proxy.async_get(p.first, p.second, args->tmp, 
						boost::bind(f, boost::asio::placeholders::error, args, cb));
			}
		}

		template <typename T>
		bool updater::add(const std::string& var, const std::string& remote_var,
						  T& local_value)
		{
			void (*f) (ability& a, 
					   const std::string& var, T& local_value,
					   network::identifier id, const std::string& src,
					   cb_type cb) = details::call_proxy<T>;
			// copy remote_var or we will have dangling reference
			fun_type fun = boost::bind(f, boost::ref(a), remote_var,
										  boost::ref(local_value), _1, _2, _3);
			std::pair<map_type::iterator, bool> p;
			p = map.insert(std::make_pair(var, fun));
			return p.second;
		}

		template <typename A>
		class update_variables<A, 0, boost::mpl::vector<> >
		{
			public:
				template <typename Handler>
				void async_update(Handler handler)
				{
					return handler(boost::system::error_code(), error());
				}

				network::error_context error() const
				{
					return network::empty_error_context;
				}
		};

		template <typename A, size_t N>
		class update_variables<A, N, boost::mpl::vector<> >
		{
			private:
				A& a;
				local_vars update_status;

			public:
				update_variables(A& a, const boost::array<std::string, N>& update) :
					a(a), update_status(update) {}

				template <typename Handler>
				void async_update(Handler handler)
				{
					a.updater.async_update(update_status, 0, a.name, handler);
				}

				network::error_context error() const
				{
					return update_status.error();
				}
		};

		template <typename A, typename vectorT>
		class update_variables<A, 0, vectorT> 
		{
			public:
				typedef model::remote_values<vectorT> remote_values;
				typedef typename remote_values::seqReturn seqReturn;
				typedef typename remote_values::remote_vars_conf remote_vars_conf;

			private:
				remote_proxy proxy;
				remote_values remote;

				template <typename Handler>
				void handle_update(const boost::system::error_code& e,
							boost::tuple<Handler> handler)
				{
					boost::get<0>(handler)(e, error());
				}

			public:
				update_variables(A& a, const typename remote_values::remote_vars_conf& vars):
					proxy(a), remote(vars)
				{}

				template <typename Handler>
				void async_update(Handler handler)
				{
					void (update_variables::*f) (const boost::system::error_code& e, 
												 boost::tuple<Handler>)
						= &update_variables::template handle_update<Handler>;
					proxy.async_get(remote, 
						boost::bind(f, this, boost::asio::placeholders::error,
											 boost::make_tuple(handler)));
				}

				template<size_t i>
				typename boost::mpl::at<seqReturn, boost::mpl::int_<i> >::type
				at_c() const
				{
					return remote.template at_c<i>();
				}

				network::error_context error() const
				{
					return remote.error();
				}
		};

		template <typename A, size_t N, typename vectorT>
		class update_variables 
		{
			public:
				typedef model::remote_values<vectorT> remote_values;
				typedef typename remote_values::seqReturn seqReturn;
				typedef typename remote_values::remote_vars_conf remote_vars_conf;

			private:
				A& a;
				remote_proxy proxy;
				local_vars local_update_status;
				remote_values remote_update_status;

				template <typename Handler>
				void handle_update(const boost::system::error_code& e, 
								   boost::tuple<Handler> handler)
				{
					if (local_update_status.is_terminated() && 
						remote_update_status.is_terminated()) {
						if (remote_update_status.is_valid()) 
							boost::get<0>(handler)(e, error());
						else
							boost::get<0>(handler)(
								make_error_code(boost::system::errc::invalid_argument), error());
					}
				}

				template <typename Handler>
				void handle_update2(const boost::system::error_code& e,
									const hyper::network::error_context&,
									boost::tuple<Handler> handler)
				{
					return handle_update(e, handler);
				}
								   

			public:
				update_variables(A& a, const boost::array<std::string, N>& update,
								 const typename remote_values::remote_vars_conf& vars):
					a(a), proxy(a), local_update_status(update),
					remote_update_status(vars)
				{}
				
				template <typename Handler>
				void async_update(Handler handler)
				{
					void (update_variables::*f) (const boost::system::error_code& e, 
												 boost::tuple<Handler>)
						= &update_variables::template handle_update<Handler>;

					void (update_variables::*f2) (const boost::system::error_code& e, 
												  const hyper::network::error_context&,
												 boost::tuple<Handler>)
						= &update_variables::template handle_update2<Handler>;

					local_update_status.reset();
					remote_update_status.reset();
					a.updater.async_update(local_update_status, 0, a.name, 
						boost::bind(f2, this, boost::asio::placeholders::error, _2,
											 boost::make_tuple(handler)));
					proxy.async_get(remote_update_status, 
						boost::bind(f, this, boost::asio::placeholders::error,
											 boost::make_tuple(handler)));
				}

				template<size_t i>
				typename boost::mpl::at<seqReturn, boost::mpl::int_<i> >::type
				at_c() const
				{
					return remote_update_status.template at_c<i>();
				}

				network::error_context error() const
				{
					network::error_context err1, err2;
					err1 = local_update_status.error();
					err2 = remote_update_status.error();
					err1.insert(err2.begin(), err2.end(), err1.end());
					return err1;
				}
		};
	}
}

#endif /* HYPER_MODEL_UPDATE_IMPL_HH */
