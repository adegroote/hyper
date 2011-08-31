
#ifndef _LOGIC_ENGINE_HH_
#define _LOGIC_ENGINE_HH_

#include <map>

#include <logic/facts.hh>
#include <logic/logic_var.hh>
#include <logic/rules.hh>

#include <boost/logic/tribool.hpp>

namespace hyper {
	namespace logic {

		struct facts_ctx {
			private:
				bool ctx_rules_update;

			public:
				facts f;

				typedef std::set<expression> expressionS;
				typedef std::map<std::string, expressionS> expressionM;
				typedef std::map<rule::identifier_type, expressionM> expressionMM;

				/*
				 * For each rule (indexed by rules::identifier_type), associate the
				 * list of possible expression for each variable. The list of
				 * possible expression is defined by the facts database, and the
				 * rules extracted from rule::symbol_to_fun.
				 */
				expressionMM symbol_to_possible_expression;

				facts_ctx(const funcDefList& fun) : ctx_rules_update(false), f(fun)
				{}

				bool add(const std::string& fact) { 
					bool res = f.add(fact);
					ctx_rules_update = false;
					return res;
				}

				bool add(const function_call& fact) { 
					bool res = f.add(fact);
					ctx_rules_update = false;
					return res;
				}

				void new_rule() { ctx_rules_update = false; }

				void compute_possible_expression(const rules& rs);
		};

		std::ostream& operator << (std::ostream& oss, const facts_ctx& f);

		class engine {
			private:
				/* A set of known function definition */
				funcDefList funcs_;

				/* 
				 * A set of facts database (our post-conditions), indexed by
				 * their context (normally task). The default context is called
				 * "default", but must be empty outside of test 
				 */
				typedef std::map<std::string, facts_ctx> factsMap;
				factsMap facts_ ;

				/* A set of logic rules, the same for all context */
				rules rules_;

				void apply_rules(facts_ctx &);

				facts_ctx& get_facts(const std::string& identifier);
				void set_facts(const std::string& identifier, 
							   const facts_ctx&);

				boost::logic::tribool infer_(const logic::function_call& f, const std::string& identifier);
				boost::logic::tribool infer_(const logic::function_call& f,
											std::vector<logic::function_call>& hyps,
										    const std::string& identifier);

				bool generate_theory(facts_ctx& facts, const std::string& identifier);
			
			public:
				engine();
				bool add_type(const std::string&);
				bool add_predicate(const std::string&, size_t, const std::vector<std::string>& args_type,
															   eval_predicate* = 0);
				bool add_func(const std::string&, size_t, const std::vector<std::string>& args_type);
				template <typename FactType>
				bool add_fact(const FactType& fact,
							  const std::string& identifier = "default")
				{
					/* yes copy it, until we check the coherency */
					facts_ctx current_facts = get_facts(identifier);
					current_facts.add(fact);

					return generate_theory(current_facts, identifier);
				}
				bool add_fact(const std::vector<std::string>&, 
							  const std::string& identifier = "default");

				template <typename FactType>
				bool add_rule(const std::string& identifier,
							  const typename std::vector<FactType>& cond,
							  const typename std::vector<FactType>& action)
				{
					bool res = rules_.add(identifier, cond, action);

					// XXX rewrite it using boost::phoenix::bind
					for (factsMap::iterator it = facts_.begin(); it != facts_.end(); ++it) 
						apply_rules(it->second);
					return res;
				}

				/* Check if goal is directly inferable in the current engine context */
				/* GoalType can be a string representing a function_call, or
				 * directly a function_call */
				template <typename GoalType>
				boost::logic::tribool infer(const GoalType& goal,
											const std::string& identifier = "default")
				{
					facts_ctx& current_facts = get_facts(identifier);
					return infer_(current_facts.f.generate(goal), identifier);
				}

				/* Check if goal is directly inferable in the current engine context.
				 * If it is not the case, hyps may contains hypothesis. If ANY of this 
				 * hypothesis can be prooved, we can infer goal from the current context
				 * or in other work,
				 *    goal is true in the current context IF ANY of hyps is true
				 */
				template <typename GoalType>
				boost::logic::tribool infer(const GoalType& goal,
											std::vector<logic::function_call>& hyps,
											const std::string& identifier = "default")
				{
					facts_ctx& current_facts = get_facts(identifier);
					return infer_(current_facts.f.generate(goal), hyps, identifier);
				}

				/*
				 * Fill OutputIterator with the list of identifier which
				 * matches the goal
				 */
				template <typename GoalType, typename OutputIterator>
				OutputIterator infer_all(const GoalType& goal, OutputIterator out)
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

				struct plausible_hypothesis {
					std::string name;
					std::vector<function_call> hyps;
				};

				template <typename GoalType, typename OutputIterator1, typename OutputIterator2>
				void infer_all(const GoalType& goal, OutputIterator1 out1, OutputIterator2 out2)
				{
					for (factsMap::const_iterator it = facts_.begin();
												  it != facts_.end(); ++it)
					{
						plausible_hypothesis h;
						h.name = it->first;
						boost::logic::tribool b = infer(goal, h.hyps, it->first);
						if (!boost::logic::indeterminate(b) && b)
							*out1++ = it->first;
						else if (boost::logic::indeterminate(b) && !h.hyps.empty())
							*out2++ = h;
					}
				}


				friend std::ostream& operator << (std::ostream&, const engine&);

				const funcDefList& funcs() const { return funcs_; }
		};

		std::ostream& operator << (std::ostream&, const engine&);

	
		bool is_world_consistent(const rules& rs, facts_ctx& facts);
	}
}

#endif /* _LOGIC_ENGINE_HH_  */
