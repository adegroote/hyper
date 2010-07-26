#ifndef _LOGIC_EVAL_HH_
#define _LOGIC_EVAL_HH_

#include <logic/expression.hh>

#include <boost/logic/tribool.hpp>

namespace hyper {
	namespace logic {

		using boost::logic::tribool;
		using boost::logic::indeterminate;

		struct notAPredicate {};

		template <typename F, int N> struct eval;

		struct eval_predicate
		{
			virtual tribool operator()(const std::vector<expression>& e) = 0;
		};

		template <int N>
		struct eval<notAPredicate, N> : eval_predicate
		{
			tribool operator() (const std::vector<expression>& e)
			{
				(void)e; return indeterminate;
			}
		};

		template <typename F>
		struct eval<F, 0> : eval_predicate
		{
			tribool operator() (const std::vector<expression>& e)
			{
				assert(e.size() == 0);
				return F()();
			}
		};

		template <>
		struct eval<notAPredicate, 0> : eval_predicate
		{
			tribool operator() (const std::vector<expression>& e)
			{
				(void)e; return indeterminate;
			}
		};

		template <typename F>
		struct eval<F, 1> : eval_predicate
		{
			tribool operator() (const std::vector<expression>& e)
			{
				assert(e.size() == 1);
				return F()(e[0].expr);
			}
		};

		template <>
		struct eval<notAPredicate, 1> : eval_predicate
		{
			tribool operator() (const std::vector<expression>& e)
			{
				(void)e; return indeterminate;
			}
		};

		template <typename F>
		struct eval<F, 2> : eval_predicate
		{
			tribool operator() (const std::vector<expression>& e)
			{
				assert(e.size() == 2);
				return F()(e[0].expr, e[1].expr);
			}
		};

		template <>
		struct eval<notAPredicate, 2> : eval_predicate
		{
			tribool operator() (const std::vector<expression>& e)
			{
				(void)e; return indeterminate;
			}
		};
	}
}

#endif /*_LOGIC_EVAL_HH_ */
