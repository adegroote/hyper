#ifndef HYPER_MODEL_UPDATE_IMPL_HH
#define HYPER_MODEL_UPDATE_IMPL_HH

#include <compiler/scope.hh>
#include <model/ability.hh>
#include <model/update.hh>

namespace hyper {
	namespace model {

		namespace details {

			template <typename T>
			struct remote_args {
				typedef network::actor::remote_proxy<model::ability> ability_remote_proxy;
					
				T& value_to_bind;
				ability_remote_proxy proxy;
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
					return cb(e);
				}

				if (args->tmp) {
					args->value_to_bind = *(args->tmp);
					delete args;
					return cb(boost::system::error_code());
				}

				delete args;
				// XXX need better error handling
				return cb(boost::asio::error::invalid_argument); 
			}

			template <typename T>
			void call_proxy(ability &a, 
							const std::string& var, T& local_value, 
							updater::cb_type cb)
			{
				void (*f) (const boost::system::error_code&, 
						   remote_args<T>* args,
						   updater::cb_type cb) = cb_proxy<T>;

				remote_args<T>* args(new remote_args<T>(a, local_value));

				std::pair<std::string, std::string> p =
							compiler::scope::decompose(var);

				updater::cb_type local_cb = 
					boost::bind(f, boost::asio::placeholders::error, 
								args, cb);

				return args->proxy.async_get(p.first, p.second, args->tmp, local_cb);
			}
		}

		template <typename T>
		bool updater::add(const std::string& var, const std::string& remote_var,
						  T& local_value)
		{
			void (*f) (ability& a, 
					   const std::string& var, T& local_value,
					   cb_type cb) = details::call_proxy<T>;
			// copy remote_var or we will have dangling reference
			fun_type fun = boost::bind(f, boost::ref(a), remote_var,
										  boost::ref(local_value), _1);
			std::pair<map_type::iterator, bool> p;
			p = map.insert(std::make_pair(var, fun));
			return p.second;
		}
	}
}

#endif /* HYPER_MODEL_UPDATE_IMPL_HH */
