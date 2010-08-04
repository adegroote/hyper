#ifndef _LOGIC_UNIFY_HH_
#define _LOGIC_UNIFY_HH_

#include <iostream>
#include <vector>

#include <logic/expression.hh>

namespace hyper {
	namespace logic {
		typedef std::map<std::string, expression> unifyM;
		typedef std::pair<bool, unifyM> unify_res;

		/*
		 * It is not a real unification in the logical sense, as it is not a
		 * symetrical operation. This unify() function tries to bind argument
		 * of f1 with arguments of f2. Maybe we need a better name here.
		 *
		 * unify(equal(a, b), equal(x, 7))
		 * will return <true, [<a,x>, <b, 7>]>
		 *
		 * but
		 * unify(equal(x, 7), equal(a, b))
		 * will return <false, ...> because 7 can't take the value of b. A real
		 * unify here will say that b == 7 and a == x with no contrainst
		 */
		unify_res unify(const function_call& f1, const function_call& f2, const unifyM& ctx);

		std::ostream& operator << (std::ostream& os, const unifyM& m);
	}
}

#endif /* _LOGIC_UNIFY_HH_ */
