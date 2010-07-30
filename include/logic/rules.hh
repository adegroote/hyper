
#ifndef _LOGIC_RULES_HH_
#define _LOGIC_RULES_HH_ 

#include <logic/expression.hh>

#include <iostream>
#include <vector>

namespace hyper {
	namespace logic {

		struct rule {
			std::string identifier;
			std::vector<function_call> condition;
			std::vector<function_call> action;
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

				size_t size() { return r_.size(); }
		};
	}
}

#endif /* _LOGIC_RULES_HH_ */
