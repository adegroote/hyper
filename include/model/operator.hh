#ifndef HYPER_MODEL_OPERATOR_HH_
#define HYPER_MODEL_OPERATOR_HH_

#include <string>

#include <logic/eval.hh>
#include <logic/expression.hh>
#include <logic/function_def.hh>
#include <model/execute_impl.hh>

#include <boost/logic/tribool.hpp>

namespace hyper {
	namespace model {
		
		namespace details {
			template <typename T>
			struct equal {
				typedef bool result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static bool apply(const T& t1, const T& t2) 
				{
					return t1 == t2;
				}
			};

			template <typename T>
			struct nequal {
				typedef bool result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static bool apply(const T& t1, const T& t2) 
				{
					return !(t1 == t2);
				}
			};

			template <typename T>
			struct add {
				typedef T result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static T apply(const T& t1, const T& t2) 
				{
					return (t1 + t2);
				}
			};

			template <typename T>
			struct minus {
				typedef T result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static T apply(const T& t1, const T& t2) 
				{
					return (t1 - t2);
				}
			};

			template <typename T>
			struct times {
				typedef T result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static T apply(const T& t1, const T& t2) 
				{
					return (t1 * t2);
				}
			};

			template <typename T>
			struct divides {
				typedef T result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static T apply(const T& t1, const T& t2) 
				{
					return (t1 / t2);
				}
			};

			template <typename T>
			struct negate {
				typedef T result_type;
				typedef typename boost::mpl::vector<T> args_type;
				static T apply(const T& t1) 
				{
					return -t1;
				}
			};

			template <typename T>
			struct less {
				typedef bool result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static bool apply(const T& t1, const T& t2) 
				{
					return (t1 < t2);
				}
			};

			template <typename T>
			struct less_equal {
				typedef bool result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static bool apply(const T& t1, const T& t2) 
				{
					return (t1 <= t2);
				}
			};

			template <typename T>
			struct greater {
				typedef bool result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static bool apply(const T& t1, const T& t2) 
				{
					return !(t1 <= t2);
				}
			};

			template <typename T>
			struct greater_equal {
				typedef bool result_type;
				typedef typename boost::mpl::vector<T, T> args_type;
				static bool apply(const T& t1, const T& t2) 
				{
					return !(t1 < t2);
				}
			};

			struct logic_nequal {
				template <typename T>
				static bool apply(const T& t1, const T& t2)
				{
					return t1 != t2;
				}
			};

			struct logic_less {
				template <typename T>
					static bool apply(const T& t1, const T& t2)
					{
						return t1 < t2;
					}
			};

			struct logic_less_equal {
				template <typename T>
					static bool apply(const T& t1, const T& t2)
					{
						return t1 <= t2;
					}
			};

			struct logic_greater {
				template <typename T>
					static bool apply(const T& t1, const T& t2)
					{
						return !(t1 <= t2);
					}
			};

			struct logic_greater_equal {
				template <typename T>
					static bool apply(const T& t1, const T& t2)
					{
						return !(t1 <= t2);
					}
			};

			template <typename Op>
			struct logic_eval_ : public boost::static_visitor<boost::logic::tribool>
			{
				template <typename T, typename U>
				boost::logic::tribool operator()(const T&, const U&) const
				{
					return boost::logic::indeterminate;
				}

				template <typename U>
				boost::logic::tribool operator () (const hyper::logic::Constant<U>& u, 
												   const hyper::logic::Constant<U>& v) const
				{
					return (Op::template apply<U>(u.value, v.value));
				}
			};

			template <typename Op>
			struct logic_eval 
			{
				boost::logic::tribool operator() (const hyper::logic::expression& e1, 
												  const hyper::logic::expression& e2) const
				{
					return boost::apply_visitor(logic_eval_<Op>(), e1.expr, e2.expr);
				}
			};

			inline
			void add_transitivy_rule(hyper::logic::engine& e, const std::string& s)
			{
				std::string rule_name = s + "_transitivity";
				std::vector<std::string> cond;
				std::vector<std::string> action;

				cond.push_back(s + "(A,B)");
				cond.push_back(s + "(B,C)");
				action.push_back(s + "(A,C)");

				e.add_rule(rule_name, cond, action);
			}

			inline
			void add_symetry_rule(hyper::logic::engine& e, const std::string& s, const std::string& type)
			{
				std::string rule_name = s + "_symetry";
				std::vector<std::string> cond;
				std::vector<std::string> action;

				cond.push_back(s + "(A,B)");

				std::ostringstream oss;
				oss << "equal_" << type << "(" << s << "(A,B), " << s << "(B, A))";
				action.push_back(oss.str());

				e.add_rule(rule_name, cond, action);
			}

			/* if First(A, B) then Second(A, B) */
			inline
			void add_induction_rule(hyper::logic::engine& e, const std::string& first, const std::string& second)
			{
				std::string rule_name = first + "_induction_" + second;
				std::vector<std::string> cond;
				std::vector<std::string> action;

				cond.push_back(first + "(A,B)");
				action.push_back(second + "(A,B)");

				e.add_rule(rule_name, cond, action);
			}
		}


		template <typename T>
		void add_equalable_type(model::functions_map& fMap, std::string typeName)
		{
			fMap.add("equal_" + typeName, new function_execution<details::equal<T> >());
			fMap.add("not_equal_" + typeName, new function_execution<details::nequal<T> > ());
		}

		inline
		void add_logic_equalable_type(logic::engine& e, std::string typeName)
		{
			e.add_type(typeName);
			std::vector<std::string> v(2, typeName);
			e.add_predicate("not_equal_" + typeName, 2, v, new logic::eval<
									details::logic_eval<details::logic_nequal>, 2>());
		}

		template <typename T>
		void add_numeric_type(model::functions_map& fMap, std::string typeName)
		{
			fMap.add("add_" + typeName, new function_execution<details::add<T> > ());
			fMap.add("minus_" + typeName, new function_execution<details::minus<T> >());
			fMap.add("times_" + typeName, new function_execution<details::times<T> >());
			fMap.add("divides_" + typeName, new function_execution<details::divides<T> >());
			fMap.add("negate_" + typeName, new function_execution<details::negate<T> >());
		}

		inline
		void add_logic_numeric_type(logic::engine& e, std::string typeName)
		{
			std::vector<std::string> v(3, typeName);
			e.add_func("add_" + typeName, 2, v);
			e.add_func("minus_" + typeName, 2, v);
			e.add_func("times_" + typeName, 2, v);
			e.add_func("divides_" + typeName, 2, v);
			v.pop_back();
			e.add_func("negate_" + typeName, 1, v);

			details::add_symetry_rule(e, "add_" + typeName, typeName);
			details::add_symetry_rule(e, "minus_" + typeName, typeName);
			details::add_symetry_rule(e, "times_" + typeName, typeName);
			details::add_symetry_rule(e, "divides_" + typeName, typeName);
		}

		template <typename T>
		void add_comparable_type(model::functions_map& fMap, std::string typeName)
		{
			fMap.add("less_" + typeName, new function_execution<details::less<T> > ());
			fMap.add("less_equal_" + typeName, new function_execution<details::less_equal<T> >());
			fMap.add("greater_" + typeName, new function_execution<details::greater<T> >());
			fMap.add("greater_equal" + typeName, new function_execution<details::greater_equal<T> >());
		}

		inline
		void add_logic_comparable_type(logic::engine& e, std::string typeName)
		{
			std::vector<std::string> v(2, typeName);
			e.add_predicate("less_" + typeName, 2, v, new logic::eval<
											details::logic_eval<details::logic_less>, 2>());
			e.add_predicate("less_equal_" + typeName, 2, v, new logic::eval<
											details::logic_eval<details::logic_less_equal>, 2>());
			e.add_predicate("greater_" + typeName , 2, v, new logic::eval<
											details::logic_eval<details::logic_greater>, 2>());
			e.add_predicate("greater_equal_" + typeName , 2, v, new logic::eval<
											details::logic_eval<details::logic_greater_equal>, 2>());

			details::add_transitivy_rule(e, "less_" + typeName);
			details::add_transitivy_rule(e, "less_equal_" + typeName);
			details::add_transitivy_rule(e, "greater_" + typeName);
			details::add_transitivy_rule(e, "greater_equal_" + typeName);

			details::add_induction_rule(e, "less_" + typeName, "less_equal_" + typeName);
			details::add_induction_rule(e, "greater_" + typeName, "greater_equal_" + typeName);
		}
	}
}

#endif /* HYPER_MODEL_OPERATOR_HH_ */
