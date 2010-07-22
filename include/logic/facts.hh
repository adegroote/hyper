#ifndef _LOGIC_FACTS_HH_
#define _LOGIC_FACTS_HH_

#include <logic/expression.hh>

#include <vector>

namespace hyper {
	namespace logic {
		class facts {

			private:
				const funcDefList& funcs;
				typedef std::vector<function_call> expressionV;

				expressionV list;

			public:	
				facts(const funcDefList& funcs_): funcs(funcs_) {}

				void add(const std::string& s)
				{
					generate_return r = generate(s, funcs);
					assert(r.res);
					list.push_back(r.e);
				}

				bool matches(const function_call & e) const;
		};
	}
}

#endif /* _LOGIC_FACTS_HH_ */
