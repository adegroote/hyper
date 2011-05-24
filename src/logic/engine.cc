#include <logic/engine.hh>
#include <logic/unify.hh>
#include <logic/eval.hh>
#include <logic/backward_chaining.hh>

#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/logic/tribool_io.hpp>

#include <set>
#include <sstream>

#include <utils/algorithm.hh>

namespace phx = boost::phoenix;

namespace {
	using namespace hyper::logic;

	/*
	 * Try to find unification between one condition of a rule and one fact
	 * If the unification is succesful, it is addede to unify_vect
	 * Otherwise, there is no effect
	 */
	struct apply_unification_ 
	{
		const facts& facts_;
		std::vector<unifyM>& unify_vect;
		const function_call &f;

		apply_unification_(const facts& facts__, std::vector<unifyM>& unify_vect__,
						   const function_call &f_):
			facts_(facts__), unify_vect(unify_vect__), f(f_)
		{}

		void operator() (const  unifyM& m)
		{
			functionId id = f.id;
			for (facts::const_iterator it = facts_.begin(id) ; it != facts_.end(id); ++it)
			{
				unify_res r = unify(f, *it, m);
				if (r.first)
					unify_vect.push_back(r.second);
			}
		}
	};

	/*
	 * Try to find unification between one condition of a rule and the facts
	 */
	struct apply_unification 
	{
		const facts& facts_;
		std::vector<unifyM>& unify_vect;

		apply_unification(const facts& facts__, std::vector<unifyM>& unify_vect__):
			facts_(facts__), unify_vect(unify_vect__)
		{}

		void operator() (const function_call& f)
		{
			std::vector<unifyM> tmp;
			std::for_each(unify_vect.begin(), unify_vect.end(), 
						  apply_unification_(facts_, tmp, f));

			unify_vect = tmp;
		}
	};

	/*
	 * Really generate a fact from an expression and a unification map
	 */
	struct do_unification : public boost::static_visitor<expression>
	{
		const unifyM& m;

		do_unification(const unifyM& m_) : m(m_) {}

		template <typename T> 
		expression operator() (const T& t) const
		{
			return t;
		}

		expression operator() (const std::string& s) const
		{
			unifyM::const_iterator it = m.find(s);
			if (it == m.end()) {
				assert(false);
				return s; // maybe assert(false) here
			}
			return it->second;
		}

		expression operator() (const function_call& f) const
		{
			function_call res = f;
			std::vector<expression>::iterator it;
			for (it = res.args.begin(); it != res.args.end(); ++it)
				*it = boost::apply_visitor(do_unification(m), it->expr);
			return res;
		}
	};

	/*
	 * Generate a fact from a rule and a unification map
	 */
	struct generate_fact
	{
		const function_call& f;

		generate_fact(const function_call& f_) : f(f_) {}

		function_call operator() (const unifyM& m) const
		{
			function_call res = f;
			std::vector<expression>::iterator it;
			for (it = res.args.begin(); it != res.args.end(); ++it)
				*it = boost::apply_visitor(do_unification(m), it->expr);
			return res;
		}
	};

	/*
	 * Add a fact from a rule and each unification we can find
	 */
	struct add_facts
	{
		facts& facts_;
		const std::vector<unifyM>& unify_vect;

		add_facts(facts& facts__, const std::vector<unifyM>& unify_vect_):
			facts_(facts__), unify_vect(unify_vect_)
		{}

		void operator() (const function_call& f)
		{
			std::set<function_call> s;

			std::transform(unify_vect.begin(), unify_vect.end(),
						   std::inserter(s, s.begin()),
						   generate_fact(f));

			bool (facts::*add_f) (const function_call& f) = & facts::add;
			std::for_each(s.begin(), s.end(), 
						  boost::bind(add_f, boost::ref(facts_), _1));
		}
	};

	std::vector<unifyM> 
	compute_rule_unification(const rule& r, const facts_ctx& facts)
	{
		// generate an empty ctx for starting the algorithm
		std::vector<unifyM> unify_vect(1);

		/* 
		 * apply_unification will try to find some unification between facts
		 * and one condition, refining unifyM context at each condition. 
		 */
		std::for_each(r.condition.begin(), r.condition.end(), 
					  apply_unification(facts.f, unify_vect));

		return unify_vect;
	}

	struct lead_to_inconsistency {
		const facts_ctx& facts;

		lead_to_inconsistency(const facts_ctx& facts) : facts(facts) {}

		bool operator() (const rule& r)
		{
			std::vector<unifyM> vec = compute_rule_unification(r, facts);
			return !vec.empty();
		}
	};


	/*
	 * Try to apply a rule to a set of facts
	 *
	 * For that, we try to compute unification between the rules condition and
	 * the facts, and if we can find such unification, we generate new facts on
	 * the base of the action part of the rule
	 *
	 * It returns true if we compute new facts
	 */
	struct apply_rule 
	{
		facts_ctx& facts;

		apply_rule(facts_ctx& facts) : facts(facts) {};

		bool operator() (const rule& r)
		{
			std::vector<unifyM> unify_vect = compute_rule_unification(r, facts);

			// generating new fact
			size_t facts_size = facts.f.size();
			std::for_each(r.action.begin(), r.action.end(),
						  add_facts(facts.f, unify_vect));

			return (facts.f.size() != facts_size);
		}
	};

	struct compute_possible_expression
	{
		const rule& r;
		facts_ctx& ctx;

		compute_possible_expression(const rule& r, facts_ctx& ctx) : 
			r(r), ctx(ctx) 
		{}

		void operator() (const std::string& s)
		{
			rule::map_symbol::const_iterator it = r.symbol_to_fun.find(s);
			assert(it != r.symbol_to_fun.end());
			const std::set<functionId> & f_id = it->second;
			assert(f_id.size() >= 1);

			std::set<functionId>::const_iterator it_id = f_id.begin();

			/* 
			 * initialize the expressionS with the sub_expression from the
			 * first fct category
			 */
			facts_ctx::expressionS res;
			res.insert(ctx.f.sub_begin(*it_id), ctx.f.sub_end(*it_id));
			++it_id;

			/*
			 * And now compute the intersection with the other sub_expression
			 */
			while (it_id != f_id.end())
			{
				facts_ctx::expressionS tmp;
				std::set_intersection(res.begin(), res.end(),
									  ctx.f.sub_begin(*it_id),
									  ctx.f.sub_end(*it_id),
									  std::inserter(tmp, tmp.begin()));
				std::swap(res, tmp);
				++it_id;
			}

			ctx.symbol_to_possible_expression[r.identifier][s] = res;
		}
	};

	struct compute_possible_expression_helper 
	{
		facts_ctx& ctx;

		compute_possible_expression_helper(facts_ctx& ctx) : ctx(ctx) {}

		void operator() (const rule &r) 
		{
			std::for_each(r.symbols.begin(), r.symbols.end(), compute_possible_expression(r, ctx));
		}
	};


	struct are_equal : public boost::static_visitor<tribool>
	{
		template <typename T, typename U>
		tribool operator()(const T& t, const U& u) const
		{
			(void)t; (void) u; 
			return boost::logic::indeterminate;
		}
	
		template <typename U>
		tribool operator () (const Constant<U>& u, const Constant<U>& v) const
		{
			return (u.value == v.value);
		}
	};

	struct equal
	{
		tribool operator() (const expression& e1, const expression& e2) const
		{
			return boost::apply_visitor(are_equal(), e1.expr, e2.expr);
		}
	};
}

namespace hyper {
	namespace logic {
		engine::engine() : rules_(funcs_) 
		{
			funcs_.add("equal", 2, new eval<equal, 2>());

			add_rule("equal_reflexivity", 
					 boost::assign::list_of<std::string>("equal(X, Y)"),
					 boost::assign::list_of<std::string>("equal(Y, X)"));

			add_rule("equal_transitiviy", 
					  boost::assign::list_of<std::string>("equal(X, Y)")("equal(Y,Z)"),
					  boost::assign::list_of<std::string>("equal(X, Z)"));
		}

		bool engine::add_predicate(const std::string& name, size_t arity,
							  eval_predicate* eval)
		{
			funcs_.add(name, arity, eval);
			/*
			 * For each function f of arity n, state that 
			 * if f(X1, X2, ..., XN) and equal(X1, Y1) then f(Y1, X2, ..., XN)
			 */
			typedef std::vector<std::string> statement;
			std::vector<statement> conds;
			for (size_t i = 0; i < arity; ++i) {
				statement s;
				std::ostringstream oss;
				oss << name << "(";
				for (size_t j = 0; j < arity; ++j) {
					oss << "X" << j;
					if (j != arity - 1)
						oss << ",";
				}
				oss << ")";
				s.push_back(oss.str());

				oss.str("");
				oss << "equal(X" << i << ",Y" << i << ")";
				s.push_back(oss.str());
				conds.push_back(s);
			}

			std::vector<statement> action;
			for (size_t i = 0; i < arity; ++i) {
				statement s;
				std::ostringstream oss;
				oss << name << "(";
				for (size_t j = 0; j < arity; ++j) {
					if ( i == j ) 
						oss << "Y" << j;
					else 
						oss << "X" << j;
					if (j != arity - 1)
						oss << ",";
				}
				oss << ")";
				s.push_back(oss.str());
				action.push_back(s);
			}

			for (size_t i = 0; i < arity; ++i) {
				std::ostringstream oss;
				oss << name << "_unify" << i;

				add_rule(oss.str(), conds[i], action[i]);
			}

			return true; // XXX
		}

		bool engine::add_func(const std::string& name, size_t arity)
		{
			funcs_.add(name, arity, 0);
			/*
			 * For each function f of arity n, state that 
			 * if f(X1, X2, ..., XN) and equal(X1, Y1) then 
			 *	equal(f(X1, X2, ..., XN), f(Y1, X2, ....));
			 */
			typedef std::vector<std::string> statement;
			std::vector<statement> conds;
			for (size_t i = 0; i < arity; ++i) {
				statement s;
				std::ostringstream oss;
				oss << name << "(";
				for (size_t j = 0; j < arity; ++j) {
					oss << "X" << j;
					if (j != arity - 1)
						oss << ",";
				}
				oss << ")";
				s.push_back(oss.str());

				oss.str("");
				oss << "equal(X" << i << ",Y" << i << ")";
				s.push_back(oss.str());
				conds.push_back(s);
			}

			std::vector<statement> action;
			for (size_t i = 0; i < arity; ++i) {
				statement s;
				std::ostringstream oss;
				oss << "equal(";
				oss << name << "(";
				for (size_t j = 0; j < arity; ++j) {
						oss << "X" << j;
					if (j != arity - 1)
						oss << ",";
				}
				oss << "),";
				oss << name << "(";
				for (size_t j = 0; j < arity; ++j) {
					if ( i == j ) 
						oss << "Y" << j;
					else 
						oss << "X" << j;
					if (j != arity - 1)
						oss << ",";
				}
				oss << "))";
				s.push_back(oss.str());
				action.push_back(s);
			}

			for (size_t i = 0; i < arity; ++i) {
				std::ostringstream oss;
				oss << name << "_equal" << i;

				add_rule(oss.str(), conds[i], action[i]);
			}

			return true; // XXX
		}

		facts_ctx & engine::get_facts(const std::string& identifier)
		{
			factsMap::iterator it = facts_.find(identifier);
			if (it == facts_.end()) {
				std::pair<factsMap::iterator, bool> p;
				p = facts_.insert(std::make_pair(identifier, facts_ctx(funcs_)));
				assert(p.second);
				return p.first->second;
			}
			return it->second;
		}

		void engine::set_facts(const std::string& identifier, const facts_ctx& ctx)
		{
			factsMap::iterator it = facts_.find(identifier);
			if (it != facts_.end()) 
				facts_.erase(it);
			facts_.insert(std::make_pair(identifier, ctx));
		}

		bool engine::add_fact(const std::string& expr,
							  const std::string& identifier)
		{

			/* yes copy it, until we check the coherency */
			facts_ctx current_facts = get_facts(identifier);
			current_facts.add(expr);
			apply_rules(current_facts);

			/* If the new fact does not lead to any inconstency, really commit
			 * it */
			if (is_world_consistent(rules_, current_facts)) {
				set_facts(identifier, current_facts);
				return true;
			} else 
				return false;
		}

		bool engine::add_fact(const std::vector<std::string>& exprs, 
							  const std::string& identifier)
		{
			// help the compiler to choose right overload
			bool (facts_ctx::*f) (const std::string& s) = &facts_ctx::add;
			facts_ctx current_facts = get_facts(identifier);
				std::for_each(exprs.begin(), exprs.end(), 
						  boost::bind(f, boost::ref(current_facts), _1));
			apply_rules(current_facts);

			if (is_world_consistent(rules_, current_facts)) {
				set_facts(identifier, current_facts);
				return true;
			} else 
				return false;
		}

		/*
		 * People who add rule are smart enough to don't add rules which lead
		 * to inconsistency in the logic, isn't it ?
		 */
		bool engine::add_rule(const std::string& identifier,
							  const std::vector<std::string>& cond,
							  const std::vector<std::string>& action)
		{
			bool res = rules_.add(identifier, cond, action);

			// XXX rewrite it using boost::phoenix::bind
			for (factsMap::iterator it = facts_.begin(); it != facts_.end(); ++it) 
				apply_rules(it->second);
			return res;
		}

		void engine::apply_rules(facts_ctx& current_facts)
		{
			current_facts.new_rule();
			rules::const_iterator it = rules_.begin();

			while (it != rules_.end())
			{
				bool new_fact = apply_rule(current_facts)(*it);
				if (new_fact) 
					it = rules_.begin();
				else
					++it;
			}
		}

		boost::logic::tribool engine::infer(const std::string& goal,
											const std::string& identifier)
		{
			facts_ctx& current_facts = get_facts(identifier);
			function_call f = current_facts.f.generate(goal);

			boost::logic::tribool b = current_facts.f.matches(f);
			if (!boost::logic::indeterminate(b))
				return b;

			bool has_concluded = false;

			current_facts.compute_possible_expression(rules_);

			backward_chaining chaining(rules_, current_facts);
			has_concluded = chaining.infer(f);
			
			if (has_concluded)
				return true;

			return boost::logic::indeterminate;
		}

		bool
		is_world_consistent(const rules& rs, facts_ctx& facts)
		{
			std::vector<rule> rules;
			hyper::utils::copy_if(rs.begin(), rs.end(), std::back_inserter(rules),
						 phx::bind(&rule::inconsistency, phx::arg_names::arg1));

			std::vector<boost::logic::tribool> matches;
			for (size_t i = 0; i < facts.f.max_id(); ++i)
				std::transform(facts.f.begin(i), facts.f.end(i), std::back_inserter(matches),
						phx::bind(&facts::matches, &facts.f, phx::arg_names::arg1));

			bool res = ! hyper::utils::any(rules.begin(), rules.end(), 
									 lead_to_inconsistency(facts));
			res = res and hyper::utils::all(matches.begin(), matches.end(),
										   std::bind2nd(std::equal_to<bool>(), true));
			return res;
		}

		std::ostream& operator << (std::ostream& oss, const facts_ctx& f)
		{
			oss << f.f;
			return oss;
		}

		std::ostream& operator << (std::ostream& os, const engine& e)
		{
			os << "FACTS : " << std::endl;
			std::map<std::string, facts_ctx>::const_iterator it;
			for (it = e.facts_.begin(); it != e.facts_.end(); ++it)
				os << "\n\t" << it->first << it->second << std::endl;
			os << "\n ====================================== \n";
			os << "RULES : " << std::endl;
			os << e.rules_;
			return os;
		}

		void facts_ctx::compute_possible_expression(const rules& rs) 
		{
			if (ctx_rules_update)
				return;

			std::for_each(rs.begin(), rs.end(), compute_possible_expression_helper(*this));
			ctx_rules_update = true;
		}
	}
}
