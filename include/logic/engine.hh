
#ifndef _LOGIC_ENGINE_HH_
#define _LOGIC_ENGINE_HH_

#include <map>

#include <logic/facts.hh>
#include <logic/logic_var.hh>
#include <logic/rules.hh>

#include <boost/logic/tribool.hpp>
#include <boost/shared_ptr.hpp>

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

				/**
				 * For each rule (indexed by rules::identifier_type), associate the
				 * list of possible expression for each variable. The list of
				 * possible expression is defined by the facts database, and the
				 * rules extracted from rule::symbol_to_fun.
				 */

				/*
				 * XXX
				 * You must be really careful when manipulating this stuff.
				 * In the current situation, it is modified once, in
				 * compute_possible_expression, but copied a lot in
				 * proof_tree::compute_node_state(), without any change on it.
				 * Considering this situation, it is correct to share
				 * symbol_to_possible_expression between the difference
				 * instances. But it may be incorrect if the implementation
				 * changes. In this case, we need to deal more smartly with
				 * this data structure, using some kind of 'persistant
				 * structure', using shared_ptr in the data structure, and
				 * replacing only part of the changed structure.
				 */
				boost::shared_ptr<expressionMM> symbol_to_possible_expression;

				facts_ctx(const funcDefList& fun) : ctx_rules_update(false), f(fun),
					symbol_to_possible_expression(new expressionMM())
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
				funcDefList funcs_; /**< A set of known function definition */

				/** 
				 * A set of facts database (our post-conditions), indexed by
				 * their context (normally task). The default context is called
				 * "default", but must be empty outside of test 
				 */
				typedef std::map<std::string, facts_ctx> factsMap;
				factsMap facts_ ;

				rules rules_;  /**< A set of logic rules, the same for all context */

				void apply_rules(facts_ctx &);

				/**
				 * Get the fact_ctx associated to an identifier
				 *
				 * @param a string identifier describing the context (corresponding to task)
				 * @return the associated fact_ctx. It is created if it does not exists.
				 */
				facts_ctx& get_facts(const std::string& identifier);


				/**
				 * Set the fact_ctx associated to an identifier
				 *
				 * @param a string identifier describing the context
				 * (corresponding to task)
				 * @param ctx 
				 * @return nothing, facts_ is modified.
				 */
				void set_facts(const std::string& identifier, const facts_ctx& ctx);

				/**
				 * Infer if a fact can be decided from the facts_ctx associated
				 * to identifier
				 *
				 * @param f represents the facts the system try to infer
				 * @param identifier is a string identifier for the fact_ctx we
				 * want to use
				 * @return a tribool true / false / can't infer
				 */
				boost::logic::tribool infer_(const logic::function_call& f, const std::string& identifier);

				/**
				 * Infer if a fact can be decided from the facts_ctx associated
				 * to identifier. If it cannot do it directly, propose some
				 * hypothesis in hyps.
				 *
				 * @param f represents the facts the system try to infer
				 * @param hyps can store some hypothesis which leads to f be true.
				 * @param identifier is a string identifier for the fact_ctx we
				 * want to use
				 * @return a tribool true / false / can't infer
				 */
				boost::logic::tribool infer_(const logic::function_call& f,
											std::vector<logic::function_call>& hyps,
										    const std::string& identifier);

				/**
				 * Compute if the fact_ctx form a sound theory or if it leads to incoherencies.
				 * In case of success, update the facts_ with facts for identifier
				 */
				bool generate_theory(facts_ctx& facts, const std::string& identifier);
			
			public:
				engine();

				/**
				 * Add the existence of type in the engine, and associated equality predicates :
				 * equal_${type}(X, Y) :- equal_${type}(Y, X)
				 * equal_${type}(X, Y), equal_${type}(Y, Z) :- equal_${type}(X, Z)
				 *
				 * @param type represents the name of the new type
				 * @return true. 
				 */
				bool add_type(const std::string& type);

				/**
				 * Add a predicate in the engine
				 *
				 * @param identifier is the name of predicate
				 * @param nb is the number of arguments
				 * @params args_types is the list of types, in order. type must exists.
				 * @param p is an implementation of the predicate, it is optional
				 *
				 * @return if the predicate is valide, and has been succesfully inserted
				 */
				bool add_predicate(const std::string& identifier, size_t nb,
								   const std::vector<std::string>& args_type,
								   eval_predicate* p= 0);
				
				/**
				 * Add a function in the engine, and its associated predicate
				 *
				 * @param identifier is the name of the function
				 * @param arity is the number of arguments
				 * @params args_types is the list of types, in order. type must exists.
				 *
				 * @return if the function is valide, and has been succesfully inserted
				 */
				bool add_func(const std::string& identifier, size_t arity, const std::vector<std::string>& args_type);


				/**
				 * Add a fact in the engine in the identifier fact_ctx. 
				 * FactType is of kind string, or logic::function_call
				 *
				 * @param fact represents the fact to insert in the engine
				 * @param identifier represents the identifier of the context
				 * where we want to insert it. The default is "default".
				 *
				 * @return the success of the insertion (if the resulting
				 * fact_ctx is coherent).
				 */
				template <typename FactType>
				bool add_fact(const FactType& fact,
							  const std::string& identifier = "default")
				{
					/* yes copy it, until we check the coherency */
					facts_ctx current_facts = get_facts(identifier);
					current_facts.add(fact);

					return generate_theory(current_facts, identifier);
				}

				/**
				 * Add a list of facts in the context identifier.
				 * @see add_fact
				 */
				bool add_fact(const std::vector<std::string>&, 
							  const std::string& identifier = "default");

				/**
				 * Add a rule in the engine. FactType is of kind string or function_call.
				 *
				 * @param identifier is the identifier rule
				 * @param cond is the list of conditions needed to trigger the rule
				 * @param action is the list of consequences of the rule
				 *
				 * @return if the rule has been successfully inserted
				 */
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

				/**
				 * Check if goal is directly inferable in the context defined
				 * by identifier GoalType can be a string representing a
				 * function_call, or directly a function_call 
				 *
				 * @param goal represents the facts the system try to infer
				 * @param identifier is a string identifier for the fact_ctx we
				 * want to use
				 *
				 * @return success / failure / can't deduce
				 *
				 * @see infer_
				 * */
				template <typename GoalType>
				boost::logic::tribool infer(const GoalType& goal,
											const std::string& identifier = "default")
				{
					facts_ctx& current_facts = get_facts(identifier);
					return infer_(current_facts.f.generate(goal), identifier);
				}

				/** 
				 * Check if goal is directly inferable in the current engine
				 * context.  If it is not the case, hyps may contains
				 * hypothesis. If ANY of this hypothesis can be prooved, we can
				 * infer goal from the current context or in other work, goal
				 * is true in the current context IF ANY of hyps is true
				 *
				 * @param goal represents the facts the system try to infer
				 * @param hyps will be the list of possible hypothesis.
				 * @param identifier is a string identifier for the fact_ctx we
				 * want to use
				 *
				 * @return success / failure / can't deduce
				 */
				template <typename GoalType>
				boost::logic::tribool infer(const GoalType& goal,
											std::vector<logic::function_call>& hyps,
											const std::string& identifier = "default")
				{
					facts_ctx& current_facts = get_facts(identifier);
					return infer_(current_facts.f.generate(goal), hyps, identifier);
				}

				/**
				 * Fill OutputIterator with the list of identifier which matches the goal
				 * GoalType is of kind string or function_call
				 * OutputIterator must match the concept of std Output Iterator
				 *
				 * @see infer
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

				/**
				 * Fill OutputIterator with the list of identifier which
				 * matches the goal, and are not in the sequence defined by
				 * begin / end
				 * GoalType is of kind string or function_call
				 * OutputIterator must match the standard concept of Output Iterator
				 * InputInterator must match the standard concept of Input Iterator 
				 *
				 * @see infer_all
				 */

				template <typename GoalType, typename OutputIterator, typename InputIterator>
				OutputIterator infer_all_in(const GoalType& goal, OutputIterator out,
											InputIterator begin, InputIterator end)
				{
					for (factsMap::const_iterator it = facts_.begin();
												  it != facts_.end(); ++it)
					{
						if (std::find(begin, end, it->first) == end) {
							boost::logic::tribool b = infer(goal, it->first);
							if (!boost::logic::indeterminate(b) && b)
								*out++ = it->first;
						}
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

				template <typename GoalType, typename OutputIterator1, typename OutputIterator2, typename InputIterator>
				void infer_all_in(const GoalType& goal, OutputIterator1 out1, OutputIterator2 out2,
								  InputIterator begin, InputIterator end)
				{
					for (factsMap::const_iterator it = facts_.begin();
												  it != facts_.end(); ++it)
					{
						if (std::find(begin, end, it->first) == end) {
							plausible_hypothesis h;
							h.name = it->first;
							boost::logic::tribool b = infer(goal, h.hyps, it->first);
							if (!boost::logic::indeterminate(b) && b)
								*out1++ = it->first;
							else if (boost::logic::indeterminate(b) && !h.hyps.empty())
								*out2++ = h;
						}
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
