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
		typedef boost::function<void (const boost::system::error_code&)> fun_cb;

		namespace details {
			void handle_nothing(const boost::system::error_code&)
			{}

			template <typename T, typename Handler>
			void  _evaluate_expression(
					boost::asio::io_service& io_s, 
					execution_context& ctx,
					const logic::function_call& f, ability &a,
					boost::optional<T>& res,
					Handler handler);

			template <typename T, typename Handler>
			void handle_evalutate_expression(
					const boost::system::error_code& e,
					function_execution_base* func,
					boost::optional<T>& res, 
					boost::tuple<Handler> handler);

			struct computation_error {};

			template <typename T>
			void handle_remote_proxy_get(const boost::system::error_code& e,
										 network::remote_proxy_base* proxy,
										 T& res,
										 fun_cb cb)
			{
				if (e) {
					cb(e);
				} else {
					boost::any res_ = proxy->extract_result();
					res = boost::any_cast<T>( res_ );
					cb(boost::system::error_code());
				}
			}

			template <typename T>
			struct evaluate_logic_expression : public boost::static_visitor<void>
			{
				model::ability& a;
				execution_context& ctx;
				boost::asio::io_service& io_s; 
				T& res;
				fun_cb cb;

				evaluate_logic_expression(boost::asio::io_service& io_s_, execution_context& ctx_,
										  model::ability & a_, T& res_, fun_cb cb_) : 
					io_s(io_s_), ctx(ctx_), a(a_), res(res_), cb(cb_) {}

				template <typename U> 
				void operator() (const U& u) const { (void) u; throw computation_error();}

				void operator() (const logic::Constant<typename T::value_type>& c) const
				{
					res = c.value;
					cb(boost::system::error_code());
				}

				void operator() (const std::string& s) const
				{
					if (!compiler::scope::is_scoped_identifier(s)) {
						boost::shared_lock<boost::shared_mutex> lock(a.mtx);
						res = a.proxy.eval<typename T::value_type>(s);
					} else {
						std::pair<std::string, std::string> p =
							compiler::scope::decompose(s);
						if (p.first == a.name) {
							boost::shared_lock<boost::shared_mutex> lock(a.mtx);
							res = a.proxy.eval<typename T::value_type>(p.second);
						} else {
							network::remote_proxy_base* proxy = 
								new network::remote_proxy<typename T::value_type, 
														  network::name_client
														 >(io_s, p.first, p.second, a.name_client);
							ctx.v_proxy.push_back(proxy);

							fun_cb local_cb = boost::bind(
									handle_remote_proxy_get<T>, 
									boost::asio::placeholders::error,
									proxy, boost::ref(res), cb);

							return proxy->async_get_result(local_cb);
						}
					}
					cb(boost::system::error_code());
				}

				void operator() (const logic::function_call& f) const
				{
					return _evaluate_expression<typename T::value_type>(io_s, ctx, f, a, res, cb);
				}
			};

				
			template <typename T>
			void evaluate(boost::asio::io_service& io_s, execution_context& ctx, 
					   const logic::expression &e, ability& a,
					   T& res, fun_cb cb)
			{
				return boost::apply_visitor(evaluate_logic_expression<T>(io_s, ctx, a, res, cb), e.expr);
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
				T* func;
				boost::asio::io_service& io_s;
				execution_context& ctx;
				const std::vector<logic::expression>& e;
				ability &a;
				fun_cb cb;

				compute(T* func_, boost::asio::io_service& io_s_, 
						execution_context& ctx_,
						const std::vector<logic::expression> & e_, ability & a_,
						fun_cb cb_) : 
					func(func_), io_s(io_s_), ctx(ctx_), e(e_), a(a_), cb(cb_) {}

				template <typename U>
				void operator() (U unused)
				{
					typedef typename boost::mpl::at< 
								typename T::real_args_type,
								U 
								>::type local_return_type;

					(void) unused;
					void (T::*f) (const boost::system::error_code&, bool &, fun_cb) =
						&T::handle_arguments_computation;

					fun_cb local_cb = boost::bind(f, func,
													 boost::asio::placeholders::error,
													 boost::ref(func->finished[U::value]),
													 cb); 
					/* 
					 * dispatch the computing of the different arguments
					 * if there are only local arguments, it doesn't change the behaviour
					 * If there are remote variables, it will improve thing, as
					 * we don't wait anymore for the completion of the first
					 * one, than the second one, just dispatch the request and
					 * wait for the answer
					 */
					io_s.dispatch(boost::bind(evaluate<local_return_type>,
										  boost::ref(io_s),
										  boost::ref(ctx),
										  boost::cref(e[U::value]),
										  boost::ref(a),
										  boost::ref(boost::get<U::value>(func->args)),
										  local_cb));
				}
			};

			template <typename T>
			struct func_finished
			{
				const T &func;
				bool & res;

				func_finished(const T& func_, bool& b): func(func_), res(b) {};

				template <typename U> // U models mpl::int_
				void operator() (U )
				{
					res = res &&  (func.finished[U::value]);
				}
			};

			template <typename T>
			struct func_computable
			{
				const T& func;
				bool & res;

				func_computable(const T& func_, bool& b): func(func_), res(b) {}

				template <typename U>
				void operator() (U )
				{
					res = res && boost::get<U::value>(func.args);
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

			template <typename T, typename Handler>
			void _evaluate_expression(
					boost::asio::io_service& io_s,
					execution_context& ctx,
					const logic::function_call& f, ability &a,
					boost::optional<T>& res,
					Handler handler
					)
			{

				function_execution_base* func = a.f_map.get(f.name);
				ctx.v_func.push_back(func);

				void (*cb)(const boost::system::error_code& e,
						  function_execution_base* func,
						  boost::optional<T>& res,
						  boost::tuple<Handler> handler) =
				handle_evalutate_expression<T, Handler>;
				

				func->async_compute(io_s, ctx, f.args, a, 
						boost::bind(cb, boost::asio::placeholders::error, 
										func, boost::ref(res), boost::make_tuple(handler)));
			}

			template <typename T, typename Handler>
			void handle_evalutate_expression(
					const boost::system::error_code& e,
					function_execution_base* func,
					boost::optional<T>& res, 
					boost::tuple<Handler> handler)
			{
				if (e)
					boost::get<0>(handler) (e);
				else {
					boost::any res_ = func->get_result();
					res = boost::any_cast< boost::optional<T> > (res_);
					boost::get<0>(handler) (boost::system::error_code());
				}		
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

			void async_compute(
				boost::asio::io_service& io_s,
				execution_context& ctx,
				const std::vector<logic::expression> &e, ability& a,
				fun_cb cb)
			{
				// compiler normally already has checked that, but ...
				assert(e.size() == args_size);

				details::compute<function_execution<T> > compute_(this, io_s, ctx, e, a, cb);
				boost::mpl::for_each<range> (compute_);
			}	

			bool is_finished() const
			{
				bool is_finished_ = true;
				details::func_finished<function_execution<T> > finish_(*this, is_finished_);
				boost::mpl::for_each<range> (finish_);

				return is_finished_;
			}

			/* 
			 * We can compute the resultat of a function only if we can compute
			 * the value of all of its arguments. If some of this arguments
			 * eval to none, then the result of the function is none
			 * (poor maybe monad)
			 */
			bool is_computable() const
			{
				bool computable = true;

				details::func_computable<function_execution<T> > computable_(*this, computable);
				boost::mpl::for_each<range> (computable_);

				return computable;
			}

			void handle_arguments_computation(const boost::system::error_code &e, bool& termined,
										      fun_cb handler)
			{
				termined = true;
				if (e) {
					handler(e);
				} else {
					if (! is_finished()) // wait for other completion
						return;

					if (is_computable())
						result = details::eval<function_execution<T>, args_size> (args) ();
					else
						result = boost::none;
					handler(boost::system::error_code());
				}
			}

			boost::any get_result() 
			{
				return boost::any(result);
			}

			virtual ~function_execution() {}
		};

		template <typename T>
		boost::optional<T> evaluate_expression(
				boost::asio::io_service& io_s, 
				const logic::function_call& f, ability &a)
		{
			try {
				execution_context ctx;
				boost::optional<T> res;
				details::_evaluate_expression<T>(io_s, ctx, f, a, res, &details::handle_nothing);
				io_s.run();
				io_s.reset();
				return res;
			} catch (details::computation_error&) {
				return boost::none;
			}

			return boost::none;
		}

		template <typename T, typename Handler>
		void async_eval_expression(
			 boost::asio::io_service& io_s,
			 execution_context& ctx,
			 const logic::function_call& f,
			 ability& a,
			 boost::optional<T> & res,
			 Handler handler)
		{
			return details::_evaluate_expression(io_s, ctx, f, a, res, handler);
		}
	}
}



#endif /* _HYPER_MODEL_EXECUTION_IMPL_HH_ */
