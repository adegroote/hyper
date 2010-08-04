
#ifndef _LOGIC_ENGINE_HH_
#define _LOGIC_ENGINE_HH_

#include <logic/facts.hh>
#include <logic/rules.hh>

#include <boost/logic/tribool.hpp>

namespace hyper {
	namespace logic {

		class engine {
			private:
				funcDefList funcs_;
				facts facts_;
				rules rules_;

				void apply_rules();

			public:
				engine();
				bool add_func(const std::string&, size_t, eval_predicate* = 0);
				bool add_fact(const std::string&);
				bool add_fact(const std::vector<std::string>&);
				bool add_rule(const std::string& identifier,
							  const std::vector<std::string>& cond,
							  const std::vector<std::string>& action);

				boost::logic::tribool infer(const std::string& goal);

				friend std::ostream& operator << (std::ostream&, const engine&);
		};

		std::ostream& operator << (std::ostream&, const engine&);
	}
}

#endif /* _LOGIC_ENGINE_HH_  */
