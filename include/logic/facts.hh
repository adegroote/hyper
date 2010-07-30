#ifndef _LOGIC_FACTS_HH_
#define _LOGIC_FACTS_HH_

#include <logic/expression.hh>

#include <vector>

namespace hyper {
	namespace logic {
		class facts {
			public:
				typedef std::vector<expression> expressionV;
				typedef std::vector<expressionV> factsV;

			private:
				const funcDefList& funcs;
				factsV list;

			public:	
				facts(const funcDefList& funcs_): funcs(funcs_) {}

				bool add(const std::string& s);

				bool matches(const function_call & e) const;
		};
	}
}

#endif /* _LOGIC_FACTS_HH_ */
