#include <logic/engine.hh>
#include <logic/unify.hh>
#include <logic/eval.hh>

#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>

#include <set>
#include <sstream>


namespace {
	using namespace hyper::logic;

	/*
	 * Try to find unification between one condition of a rule and one fact
	 * If the unification is succesful, it is addede to unify_vect
	 * Otherwise, there is no effect
	 */
	struct apply_unification_ 
	{
		facts& facts_;
		std::vector<unifyM>& unify_vect;
		const function_call &f;

		apply_unification_(facts& facts__, std::vector<unifyM>& unify_vect__,
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
		facts& facts_;
		std::vector<unifyM>& unify_vect;

		apply_unification(facts& facts__, std::vector<unifyM>& unify_vect__):
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
			if (it == m.end()) 
				return s; // maybe assert(false) here
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
			std::vector<function_call> v;

			std::transform(unify_vect.begin(), unify_vect.end(),
						   std::back_inserter(v),
						   generate_fact(f));

			bool (facts::*add_f) (const function_call& f) = & facts::add;
			std::for_each(v.begin(), v.end(), 
						  boost::bind(add_f, boost::ref(facts_), _1));
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
		facts& facts_;

		apply_rule(facts& facts) : facts_(facts) {};

		bool operator() (const rule& r)
		{
			// generate an empty ctx for starting the algorithm
			std::vector<unifyM> unify_vect(1);

			std::for_each(r.condition.begin(), r.condition.end(), 
						  apply_unification(facts_, unify_vect));

			// generating new fact
			size_t facts_size = facts_.size();
			std::for_each(r.action.begin(), r.action.end(),
						  add_facts(facts_, unify_vect));

			size_t new_facts_size = facts_.size();

			return (new_facts_size != facts_size);
		}
	};

	struct apply_goal_unification 
	{
		const function_call &f;
		std::vector<unifyM>& unify_vect;

		apply_goal_unification(const function_call& f_, std::vector<unifyM>& unify_vect_):
			f(f_), unify_vect(unify_vect_)
		{}

		void operator() (const function_call& f_rule)
		{
			unifyM m;
			unify_res r = unify(f_rule, f, m);
			if (r.first)
				unify_vect.push_back(r.second);
		}
	};

	template <typename T>
	struct list_keys 
	{
		typename std::set<T>& s;

		list_keys(typename std::set<T>& s_) : s(s_) {}

		template <typename U>
		void operator() (const typename std::pair<const T, U>& p) 
		{
			s.insert(p.first);
		}
	};

	typedef std::set<expression> expressionS;
	typedef std::map<std::string, expressionS> expressionM;
	typedef std::vector<expressionM> vec_expressionM;

	struct compute_possible_expression
	{
		facts& facts_;
		const rule& r;
		expressionM& m;

		compute_possible_expression(facts& facts__, const rule& r_, expressionM& m_) :
			facts_(facts__), r(r_), m(m_) {}

		void operator() (const std::string& s)
		{
			rule::map_symbol::const_iterator it = r.symbols.find(s);
			assert(it != r.symbols.end());
			const std::set<functionId> & f_id = it->second;
			assert(f_id.size() >= 1);

			std::set<functionId>::const_iterator it_id = f_id.begin();


			/* 
			 * initialize the expressionS with the sub_expression from the
			 * first fct category
			 */
			expressionS res;
			res.insert(facts_.sub_begin(*it_id), facts_.sub_end(*it_id));
			++it_id;

			/*
			 * And now compute the intersection with the other sub_expression
			 */
			while (it_id != f_id.end())
			{
				expressionS tmp;
				std::set_intersection(res.begin(), res.end(),
									  facts_.sub_begin(*it_id),
									  facts_.sub_end(*it_id),
									  std::inserter(tmp, tmp.begin()));
				std::swap(res, tmp);
				++it_id;
			}

			m[s] = res;
		}
	};

	struct unifyM_candidate 
	{
		const std::set<std::string>& unbounded;
		const expressionM& possible_symbol;
		std::vector<std::string> v_symbol;

		struct iter {
			expressionS::const_iterator begin;
			expressionS::const_iterator end;
			expressionS::const_iterator current;
		};

		std::vector<iter> v;

		unifyM_candidate(const std::set<std::string>& unbounded_,
						 const expressionM& possible_symbol_):
			unbounded(unbounded_), possible_symbol(possible_symbol_)
		{
			std::copy(unbounded.begin(), unbounded.end(), std::back_inserter(v_symbol));
			for (size_t i = 0; i < v_symbol.size(); ++i) {
				expressionM::const_iterator it = possible_symbol.find(v_symbol[i]);
				assert(it != possible_symbol.end());
				iter current;
				current.begin = it->second.begin();
				current.end = it->second.end();
				current.current = current.begin;
				v.push_back(current);
			}
		}

		bool next(unifyM &m)
		{
			// generate the current solution
			for (size_t i = 0; i < v_symbol.size(); ++i)
				m[v_symbol[i]] = *v[i].current;

			// next one
			size_t i = 0;
			bool add = true;

			while (add && i < v_symbol.size()) {
				++v[i].current;
				if (v[i].current == v[i].end) {
					v[i].current = v[i].begin;
					++i;
					add = true;
				} else {
					add = false;
				}
			}

			return !add;
		}
	};

	struct generate_inferred_fact{
		const unifyM& m;

		generate_inferred_fact(const unifyM& m_) : m(m_) {}

		function_call operator() (const function_call& f) 
		{
			function_call res = f;
			std::vector<expression>::iterator it;
			for (it = res.args.begin(); it != res.args.end(); ++it)
				*it = boost::apply_visitor(do_unification(m), it->expr);
			return res;
		}
	};

	bool try_to_infer(facts& facts_, const rule& r, const unifyM& m_,
					  const std::set<std::string>& unbounded,
					  const expressionM& possible_symbol)
	{
		unifyM m(m_);
		unifyM_candidate candidates(unbounded, possible_symbol);
		bool has_next = true;
		while (has_next)
		{
			has_next = candidates.next(m);
			std::vector<function_call> v_f;
			std::transform(r.condition.begin(), r.condition.end(),
						   std::back_inserter(v_f),
						   generate_inferred_fact(m));

			std::vector<function_call>::const_iterator it;
			it = v_f.begin();
			bool has_matched = true;
			while (has_matched && it != v_f.end()) {
				has_matched = facts_.matches(*it);
				++it;
			}

			if (has_matched)
				return true;
		}

		return false;
	}

	struct infer_from_rule
	{
		facts& facts_;
		const function_call& f;

		infer_from_rule(facts& facts, const function_call& f_) : facts_(facts), f(f_) {}

		bool operator() (const rule& r)
		{
			std::vector<unifyM> unify_vect;

			std::for_each(r.action.begin(), r.action.end(),
						  apply_goal_unification(f, unify_vect));

			if (unify_vect.empty())
				return false;

			typedef std::set<std::string> set_symbols;
			typedef std::vector<set_symbols> vector_set_symbols;
			set_symbols rules_symbols;

			/* transform r.symbols into a std::set<std::string> */
			std::for_each(r.symbols.begin(), r.symbols.end(), 
						  list_keys<std::string>(rules_symbols));

			/* for each bound map, list each bounded key */
			vector_set_symbols bounded_symbols(unify_vect.size());
			for (size_t i = 0; i < unify_vect.size(); ++i) 
				std::for_each(unify_vect[i].begin(), unify_vect[i].end(),
							  list_keys<std::string>(bounded_symbols[i]));

			/* for each bound map, compute all the unbounded key */
			vector_set_symbols unbounded_symbols(unify_vect.size());
			for (size_t i = 0; i < unify_vect.size(); ++i)
				std::set_difference(rules_symbols.begin(), rules_symbols.end(),
									bounded_symbols[i].begin(), bounded_symbols[i].end(),
									std::inserter(unbounded_symbols[i], unbounded_symbols[i].begin()));

			/* 
			 * For each bound map, for each unbounded key, compute the set of
			 * possible expression (intersection of all possible expression for
			 * each category
			 */
			vec_expressionM possible_expression(unify_vect.size());
			for (size_t i = 0; i < unify_vect.size(); ++i)
				std::for_each(unbounded_symbols[i].begin(),
							  unbounded_symbols[i].end(),
							  compute_possible_expression(facts_, r, possible_expression[i]));

			/* 
			 * Now, for each bound map, for each unbounded key, try to fill it
			 * with a valid expression and check if it defines some valid facts
			 * If we find such a combinaison, goal can be inferred.
			 */
			for (size_t i = 0; i < unify_vect.size(); ++i)
				if (try_to_infer(facts_, r, unify_vect[i], unbounded_symbols[i], possible_expression[i]))
					return true;

			return false;
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
		engine::engine() : facts_(funcs_), rules_(funcs_) 
		{
			funcs_.add("equal", 2, new eval<equal, 2>());

			add_rule("equal_reflexivity", 
					 boost::assign::list_of<std::string>("equal(X, Y)"),
					 boost::assign::list_of<std::string>("equal(Y, X)"));

			add_rule("equal_transitiviy", 
					  boost::assign::list_of<std::string>("equal(X, Y)")("equal(Y,Z)"),
					  boost::assign::list_of<std::string>("equal(X, Z)"));
		}

		bool engine::add_func(const std::string& name, size_t arity,
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

		bool engine::add_fact(const std::string& expr)
		{
			facts_.add(expr);
			apply_rules();
			return true;
		}

		bool engine::add_fact(const std::vector<std::string>& exprs)
		{
			// help the compiler to choose right overload
			bool (facts::*f) (const std::string& s) = &facts::add;
			std::for_each(exprs.begin(), exprs.end(), 
						  boost::bind(f, boost::ref(facts_), _1));
			apply_rules();
			return true;
		}

		bool engine::add_rule(const std::string& identifier,
							  const std::vector<std::string>& cond,
							  const std::vector<std::string>& action)
		{
			bool res = rules_.add(identifier, cond, action);
			apply_rules();
			return res;
		}

		void engine::apply_rules()
		{
			rules::const_iterator it = rules_.begin();

			while (it != rules_.end())
			{
				bool new_fact = apply_rule(facts_)(*it);
				if (new_fact) 
					it = rules_.begin();
				else
					++it;
			}
		}

		boost::logic::tribool engine::infer(const std::string& goal)
		{
			generate_return r = generate(goal, funcs_);
			assert(r.res);
			const function_call& f = r.e;

			boost::logic::tribool b = facts_.matches(f);
			if (!boost::logic::indeterminate(b))
				return b;

			rules::const_iterator it = rules_.begin();
			bool has_concluded = false;

			while (it != rules_.end())
			{
				has_concluded = infer_from_rule(facts_, f)(*it);
				if (has_concluded) 
					return true;
				++it;
			}

			return boost::logic::indeterminate;
		}

		std::ostream& operator << (std::ostream& os, const engine& e)
		{
			os << "FACTS : " << std::endl;
			os << e.facts_;
			os << "\n ====================================== \n";
			os << "RULES : " << std::endl;
			os << e.rules_;
			return os;
		}
	}
}
