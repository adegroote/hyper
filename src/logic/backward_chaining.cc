#include <logic/backward_chaining.hh>
#include <logic/unify.hh>

#include <utils/algorithm.hh>

#include <boost/spirit/home/phoenix.hpp> 
#include <boost/fusion/include/std_pair.hpp> 
#include <boost/logic/tribool_io.hpp>


namespace phx = boost::phoenix;

namespace {
	using namespace hyper::logic;

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

	struct no_candidate {};

	struct unifyM_candidate 
	{
		const std::set<std::string>& unbounded;
		const facts_ctx::expressionM& possible_symbol;
		std::vector<std::string> v_symbol;

		struct iter {
			facts_ctx::expressionS::const_iterator begin;
			facts_ctx::expressionS::const_iterator end;
			facts_ctx::expressionS::const_iterator current;
		};

		std::vector<iter> v;

		/* Throw a no_candidate exception if we don't have any candidate for
		 * one symbol. It means that the unification can't lead to any
		 * succesful result
		 */
		unifyM_candidate(const std::set<std::string>& unbounded_,
						 const facts_ctx::expressionM& possible_symbol_):
			unbounded(unbounded_), possible_symbol(possible_symbol_)
		{
			std::copy(unbounded.begin(), unbounded.end(), std::back_inserter(v_symbol));
			for (size_t i = 0; i < v_symbol.size(); ++i) {
				facts_ctx::expressionM::const_iterator it = possible_symbol.find(v_symbol[i]);
				assert(it != possible_symbol.end());
				iter current;
				current.begin = it->second.begin();
				current.end = it->second.end();
				if (current.begin == current.end)
					throw no_candidate();
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

	bool try_to_infer(facts_ctx& facts_, const rule& r, const unifyM& m_,
					  const std::set<std::string>& unbounded)
	{
		unifyM m(m_);
		try {
			unifyM_candidate candidates(unbounded, facts_.symbol_to_possible_expression[r.identifier]);
			bool has_next = true;
			while (has_next)
			{
				has_next = candidates.next(m);
				std::vector<function_call> v_f;
				std::transform(r.condition.begin(), r.condition.end(),
							   std::back_inserter(v_f),
							   generate_inferred_fact(m));

				boost::logic::tribool b;
				bool all_true = true;
				std::vector<function_call> new_hypothesis;
				std::vector<function_call>::const_iterator it;
				for (it = v_f.begin(); it != v_f.end(); ++it) {
					b = facts_.f.matches(*it);
					if (!b) 
						continue;
					// check that the hypothesis won't lead to inconsistency

					if (indeterminate(b)) {
						new_hypothesis.push_back(*it);
						all_true = false;
					}
	
					if (all_true)
						return true;
				}

			}
		} catch (no_candidate&) {}

		return false;
	}
	struct infer_from_rule
	{
		facts_ctx& facts_;
		const function_call& f;

		infer_from_rule(facts_ctx& facts, const function_call& f_) : facts_(facts), f(f_) {}

		bool operator() (const rule& r)
		{
			std::vector<unifyM> unify_vect;

			std::for_each(r.action.begin(), r.action.end(),
						  apply_goal_unification(f, unify_vect));

			if (unify_vect.empty())
				return false;

			typedef std::set<std::string> set_symbols;
			typedef std::vector<set_symbols> vector_set_symbols;

			/* for each bound map, list each bounded key */
			vector_set_symbols bounded_symbols(unify_vect.size());
			for (size_t i = 0; i < unify_vect.size(); ++i) 
				std::transform(unify_vect[i].begin(), unify_vect[i].end(),
							   std::inserter(bounded_symbols[i], bounded_symbols[i].begin()),
							   phx::at_c<0>(phx::arg_names::arg1));

			/* for each bound map, compute all the unbounded key */
			vector_set_symbols unbounded_symbols(unify_vect.size());
			for (size_t i = 0; i < unify_vect.size(); ++i) 
				std::set_difference(r.symbols.begin(), r.symbols.end(),
									bounded_symbols[i].begin(), bounded_symbols[i].end(),
									std::inserter(unbounded_symbols[i], unbounded_symbols[i].begin()));

			/* 
			 * Now, for each bound map, for each unbounded key, try to fill it
			 * with a valid expression and check if it defines some valid facts
			 * If we find such a combinaison, goal can be inferred.
			 */
			for (size_t i = 0; i < unify_vect.size(); ++i)
				if (try_to_infer(facts_, r, unify_vect[i], unbounded_symbols[i]))
					return true;

			return false;
		}
	};

}

namespace hyper { namespace logic {

	bool backward_chaining::infer(const function_call& f)
	{
		return  hyper::utils::any(rs.begin(), rs.end(),
							       infer_from_rule(ctx, f));
	}
}}
