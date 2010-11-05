#ifndef _HYPER_MODEL_EXECUTION_IMPL_HH_
#define _HYPER_MODEL_EXECUTION_IMPL_HH_

#include <iostream>
#include <map>
#include <string>

#include <compiler/scope.hh>
#include <logic/expression.hh>
#include <network/utils.hh>
#include <network/proxy.hh>
#include <model/execute.hh>

#include <boost/any.hpp>
#include <boost/array.hpp>
#include <boost/optional/optional.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#ifndef EVAL_MAX_PARAMS
#define EVAL_MAX_PARAMS 10
#endif

namespace hyper {
	namespace model {
		namespace details {
			template <typename T>
			boost::optional<T> _evaluate_expression(
					boost::asio::io_service& io_s, 
					const logic::function_call& f, ability &a);

			struct computation_error {};

			template <typename T>
			struct evaluate_logic_expression : public boost::static_visitor<T>
			{
				model::ability & a;
				boost::asio::io_service& io_s; 

				evaluate_logic_expression(boost::asio::io_service& io_s_, model::ability & a_) : 
					io_s(io_s_), a(a_) {}

				template <typename U> 
				T operator() (const U& u) const { (void) u; throw computation_error();}

				T operator() (const logic::Constant<typename T::value_type>& c) const
				{
					return c.value;
				}

				T operator() (const std::string& s) const
				{
					T value = boost::none;;
					if (!compiler::scope::is_scoped_identifier(s)) {
						boost::shared_lock<boost::shared_mutex> lock(a.mtx);
						value =a.proxy.eval<typename T::value_type>(s);
					} else {
						std::pair<std::string, std::string> p =
							compiler::scope::decompose(s);
						if (p.first == a.name) {
							boost::shared_lock<boost::shared_mutex> lock(a.mtx);
							value = a.proxy.eval<typename T::value_type>(p.second);
						} else {
							network::remote_proxy_sync<typename T::value_type, network::name_client>
								proxy(io_s, p.first, p.second, a.name_client);
							value = proxy.get(boost::posix_time::millisec(20));
						}
					}
					if (!value) 
						throw computation_error();
					return value;
				}

				T operator() (const logic::function_call& f) const
				{
					return _evaluate_expression<typename T::value_type>(io_s, f, a);
				}
			};

				
			template <typename T>
			T evaluate(boost::asio::io_service& io_s, const logic::expression &e, ability& a)
			{
				return boost::apply_visitor(evaluate_logic_expression<T>(io_s, a), e.expr);
			}

			template <typename T>
			struct to_optional
			{
				typedef typename boost::optional<T> type;
			};

			template <typename T>
			struct init_func
			{
				T & func;

				init_func(T& func_) : func(func_) {}

				template <typename U>
				void operator() (U unused)
				{
					(void) unused;
					boost::get<U::value>(func.args) = boost::none;
					func.finished[U::value] = false;
				}
			};

			template <typename T>
			struct compute
			{
				T& func;
				const std::vector<logic::expression>& e;
				boost::asio::io_service& io_s;
				ability &a;

				compute(T& func_, boost::asio::io_service& io_s_, 
						const std::vector<logic::expression> & e_, ability & a_) : 
					func(func_), io_s(io_s_), e(e_), a(a_) {}

				template <typename U>
				void operator() (U unused)
				{
					typedef typename boost::mpl::at< 
								typename T::real_args_type,
								U 
								>::type local_return_type;

					(void) unused;
					boost::get<U::value>(func.args) = evaluate<local_return_type> (io_s, e[U::value], a);
				}
			};

			template <typename T, size_t N>
			struct eval;

			/* 
			 * Generate eval definition for N between 0 and EVAL_MAX_PARAMS
			 * It just call T::internal_type::apply(...) with the right arguments list, 
			 * Boost.preprocessing is nice but not so clear to use
			 */

#define NEW_EVAL_PARAM_DECL(z, n, _) BOOST_PP_COMMA_IF(n) *boost::get<n>(args)

#define NEW_EVAL_DECL(z, n, _) \
	template<typename T> \
	struct eval<T, n> \
	{ \
		typename T::tuple_type & args; \
		eval(typename T::tuple_type & args_) : args(args_) {} \
		\
		typename T::result_type operator() () \
		{ \
			return T::internal_type::apply( \
			BOOST_PP_REPEAT(n, NEW_EVAL_PARAM_DECL, _) \
			); \
		} \
	};\

BOOST_PP_REPEAT(BOOST_PP_INC(EVAL_MAX_PARAMS), NEW_EVAL_DECL, _)

#undef NEW_EVAL_PARAM_DECL
#undef NEW_EVAL_DECL

			template <typename T>
			boost::optional<T> _evaluate_expression(
					boost::asio::io_service& io_s,
					const logic::function_call& f, ability &a)
			{
				boost::any res;
				boost::scoped_ptr<function_execution_base> func(a.f_map.get(f.name));
				func->operator() (io_s, f.args, a);
				res = func->get_result();
				// by construction, it must never throw. If it does, need to fix hyper::compiler :D
				return boost::any_cast<T> (res);
			}
		}

		template <typename T>
		struct function_execution : public function_execution_base {
			typedef T internal_type;
			typedef typename boost::optional<typename T::result_type> result_type;
			typedef typename T::args_type args_type;

			typedef typename boost::mpl::transform<
				args_type, 
				details::to_optional<boost::mpl::_1> 
			>::type real_args_type;

			typedef typename network::to_tuple<real_args_type>::type tuple_type;
			enum { args_size = boost::mpl::size<args_type>::type::value };
			typedef boost::mpl::range_c<size_t, 0, args_size> range;

			tuple_type args;
			boost::array<bool, args_size> finished;
			result_type result;

			function_execution() {
				details::init_func<function_execution<T> > init_(*this);
				boost::mpl::for_each<range> (init_);
				result = boost::none;
			}

			function_execution_base* clone() {
				return new function_execution<T>();
			}

			void operator() (
					boost::asio::io_service &io_s,
					const std::vector<logic::expression> & e, ability & a)
			{
				// compiler normally already has checked that, but ...
				assert(e.size() == args_size);

				details::compute<function_execution<T> > compute_(*this, io_s, e, a);
				boost::mpl::for_each<range> (compute_);

				result = details::eval<function_execution<T>, args_size> (args) ();
			}

			boost::any get_result() 
			{
				if (!result)
					throw details::computation_error();
				else
					return boost::any(*result);
			}

			virtual ~function_execution() {}
		};


		template <typename T>
		boost::optional<T> evaluate_expression(
				boost::asio::io_service& io_s, 
				const logic::function_call& f, ability &a)
		{
			try {
				return details::_evaluate_expression<T>(io_s, f, a);
			} catch (details::computation_error&) {
				return boost::none;
			}

			return boost::none;
		}
	}
}



#endif /* _HYPER_MODEL_EXECUTION_IMPL_HH_ */
