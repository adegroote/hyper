#ifndef HYPER_MODEL_SETTER_IMPL_HH_
#define HYPER_MODEL_SETTER_IMPL_HH_

#include <model/setter.hh>
#include <model/actor_impl.hh>

namespace hyper {
	namespace model {
		namespace setter_details {

			template <typename T>
			struct proxy {
				typedef network::actor::remote_proxy<model::actor_impl> ability_remote_proxy;

				boost::optional<T> tmp;
				ability_remote_proxy proxy_;

				proxy(hyper::model::ability& a) : proxy_(*a.actor) {}
			};

			template <typename T>
			void handle_proxy(const boost::system::error_code& e, proxy<T>* pr, T& var, setter::cb_fun cb) 
			{
				if (!e && pr->tmp)
					var = *(pr->tmp);
				delete pr;
				cb(e);
			}

			template <typename T>
			struct setter_visitor : public boost::static_visitor<void>
			{
				hyper::model::ability &a;
				T& var;
				setter::cb_fun cb;

				setter_visitor(hyper::model::ability& a, T& var, setter::cb_fun cb) : 
					a(a), var(var), cb(cb) {}

				template <typename U>
				void operator() (const U& ) const { assert(false); }

				template <typename U>
				void operator() (const logic::Constant<U>& c, 
								 typename boost::enable_if<boost::is_same<U, T> >::type* dummy = 0) const 
				{
					var = c.value;
					cb(boost::system::error_code());
				}

				void operator() (const std::string& s) const 
				{
					std::pair<std::string, std::string> p;
					p = hyper::compiler::scope::decompose(s);
					assert(p.first != a.name);

					proxy<T>* pr = new proxy<T>(a);

					void (*f) (const boost::system::error_code&, proxy<T>*, T&, setter::cb_fun) =
						&handle_proxy<T>;

					return pr->proxy_.async_get(p.first, p.second, pr->tmp, 
								boost::bind(f, boost::asio::placeholders::error,
										     pr, boost::ref(var), cb));
				}
			};

			template <typename T> 
			void setter_helper(hyper::model::ability& a, T& var, const logic::expression& e, setter::cb_fun cb)
			{
				return boost::apply_visitor( setter_visitor<T>(a, var, cb), e.expr);
			}
		}
				
		template <typename T>
		void setter::add(const std::string& s, T& var)
		{
			map_fun::const_iterator it;
			it = m.find(s);
			assert(it == m.end());
			void (*f) (model::ability&, T&, const logic::expression&, setter::cb_fun) = 
				&setter_details::setter_helper<T>;

			m[s] = boost::bind(f, boost::ref(a), boost::ref(var), _1, _2);
		}

		inline
		void setter::set(const std::string& s, const logic::expression& e, setter::cb_fun cb) const
		{
			map_fun::const_iterator it;
			it = m.find(s);
			if (it == m.end()) {
				cb(boost::asio::error::invalid_argument);
			} else
				it->second(e, cb);
		}

		inline
		void setter::set(const std::pair<std::string, const logic::expression>& p, cb_fun cb) const
		{
			return set(compiler::scope::get_identifier(p.first), p.second, cb);
		}
	}
}

#endif /* HYPER_MODEL_SETTER_IMPL_HH_ */
