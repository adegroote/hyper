#ifndef _LOGIC_FACTS_HH_
#define _LOGIC_FACTS_HH_

#include <logic/expression.hh>

#include <iostream>
#include <set>
#include <vector>

namespace hyper {
	namespace logic {
		class facts {
			public:
				typedef std::set<function_call> expressionS;
				typedef std::vector<expressionS> factsV;
				typedef expressionS::const_iterator const_iterator;

			private:
				const funcDefList& funcs;
				mutable factsV list;
				size_t size__;

			public:	
				facts(const funcDefList& funcs_): funcs(funcs_), size__(0) {}

				bool add(const std::string& s);
				bool add(const function_call& f);

				bool matches(const function_call & e) const;

				const_iterator begin(functionId id) const { 
					if (id >= list.size())
						list.resize(funcs.size());
					return list[id].begin();
				}

				const_iterator end(functionId id) const { 
					if (id >= list.size())
						list.resize(funcs.size());
					return list[id].end();
				}

				size_t size(functionId id) const {
					if (id >= list.size())
						list.resize(funcs.size());
					return list[id].size();
				}

				size_t size() const { return size__; }

				friend std::ostream& operator << (std::ostream& os, const facts&);
		};

		std::ostream& operator << (std::ostream& os, const facts&);
	}
}

#endif /* _LOGIC_FACTS_HH_ */
