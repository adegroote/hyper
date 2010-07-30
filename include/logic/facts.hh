#ifndef _LOGIC_FACTS_HH_
#define _LOGIC_FACTS_HH_

#include <logic/expression.hh>

#include <vector>

namespace hyper {
	namespace logic {
		class facts {
			public:
				typedef std::vector<function_call> expressionV;
				typedef expressionV::const_iterator const_iterator;

			private:
				const funcDefList& funcs;
				expressionV list;

			public:	
				facts(const funcDefList& funcs_): funcs(funcs_) {}

				const_iterator add(const std::string& s)
				{
					generate_return r = generate(s, funcs);
					assert(r.res);
					list.push_back(r.e);
					return --list.end();
				}

				const_iterator begin() const { return list.begin(); }
				const_iterator end() const { return list.end(); }

				bool matches(const function_call & e) const;
		};
	}
}

#endif /* _LOGIC_FACTS_HH_ */
