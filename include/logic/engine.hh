
#ifndef _LOGIC_ENGINE_HH_
#define _LOGIC_ENGINE_HH_

#include <map>

#include <logic/facts.hh>
#include <logic/rules.hh>

#include <boost/logic/tribool.hpp>

namespace hyper {
	namespace logic {

		class engine {
			private:
				funcDefList funcs_;
				typedef std::map<std::string, facts> factsMap;
				factsMap facts_ ;
				rules rules_;

				void apply_rules(facts&);

				facts& get_facts(const std::string& identifier);

			public:
				engine();
				bool add_predicate(const std::string&, size_t, eval_predicate* = 0);
				bool add_func(const std::string&, size_t);
				bool add_fact(const std::string&,
							  const std::string& identifier = "default");
				bool add_fact(const std::vector<std::string>&, 
							  const std::string& identifier = "default");
				bool add_rule(const std::string& identifier,
							  const std::vector<std::string>& cond,
							  const std::vector<std::string>& action);

				boost::logic::tribool infer(const std::string& goal, 
											const std::string& identifier = "default");

				/*
				 * Fill OutputIterator with the list of identifier which
				 * matches the goal
				 */
				template <typename OutputIterator>
				OutputIterator infer(const std::string& goal, OutputIterator out)
				{
					for (factsMap::const_iterator it = facts_.begin();
												  it != facts_.end(); ++it)
					{
						boost::logic::tribool b = infer(goal, it->first);
						if (!boost::logic::indeterminate(b) && b)
							*out++ = it->first;
					}

					return out;
				}

				friend std::ostream& operator << (std::ostream&, const engine&);
		};

		std::ostream& operator << (std::ostream&, const engine&);
	}
}

#endif /* _LOGIC_ENGINE_HH_  */
