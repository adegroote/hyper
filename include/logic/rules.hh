
#ifndef _LOGIC_RULES_HH_
#define _LOGIC_RULES_HH_ 

#include <logic/expression.hh>

#include <iostream>
#include <vector>
#include <set>

namespace hyper {
	namespace logic {

		struct rule {
			typedef std::string identifier_type;

			identifier_type identifier;
			std::vector<function_call> condition;
			std::vector<function_call> action;

			typedef std::map<std::string, std::set<functionId> > map_symbol;
			typedef std::vector<std::string> list_symbols;

			bool inconsistency() const { return action.empty(); }

			/* 
			 * A list of sorted symbols which appears in the rules. 
			 * No duplicate symbol in the list.
			 */
			list_symbols symbols;

			/*
			 * Associate to each symbol the set of function it appears in.
			 * Will be used to compute the possible value of this symbol, 
			 * considering the intersection of values of this different functions.
			 *
			 * S_{value} = Intersection_{i} S_{functionId_{i} } 
			 */
			map_symbol symbol_to_fun;

		};

		std::ostream& operator << (std::ostream&, const rule&);

		class rules {
			public:
				typedef std::vector<rule> vectorRule;
				typedef vectorRule::const_iterator const_iterator;

			private:
				const funcDefList& funcs;
				vectorRule r_; 

			public:
				rules(const funcDefList& funcs_) : funcs(funcs_) {}
				bool add(const std::string& identifier,
						 const std::vector<std::string>& cond,
						 const std::vector<std::string>& action);

				const_iterator begin() const { return r_.begin(); }
				const_iterator end() const { return r_.end(); }

				size_t size() const { return r_.size(); }


				friend std::ostream& operator << (std::ostream&, const rules&);
		};
		std::ostream& operator << (std::ostream&, const rules&);
	}
}

#endif /* _LOGIC_RULES_HH_ */
