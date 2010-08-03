#ifndef _LOGIC_FACTS_HH_
#define _LOGIC_FACTS_HH_

#include <logic/expression.hh>

#include <vector>

namespace hyper {
	namespace logic {
		class facts {
			public:
				typedef std::vector<function_call> expressionV;
				typedef std::vector<expressionV> factsV;
				typedef expressionV::const_iterator const_iterator;

			private:
				const funcDefList& funcs;
				mutable factsV list;
				size_t size__;

			public:	
				facts(const funcDefList& funcs_): funcs(funcs_), size__(0) {}

				bool add(const std::string& s);

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


		};
	}
}

#endif /* _LOGIC_FACTS_HH_ */
