#include <logic/backward_chaining.hh>
#include <logic/unify.hh>

#include <compiler/output.hh>
#include <utils/algorithm.hh>

#include <boost/bimap/bimap.hpp>
#include <boost/fusion/include/std_pair.hpp> 

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

		/* 
		 * Compute the next possible combinaison, returns false if it is the
		 * last one */
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
}

namespace hyper { namespace logic {
	namespace details {
	std::ostream& operator << (std::ostream& os, const proof_state& s)
	{
		switch (s) {
			case proven_true :
				os << "proven_true";
				return os;
			case proven_false :
				os << "proven_false";
				return os;
			case not_proven_not_explored:
				os << "not_proven_not_explored";
				return os;
			case not_proven_exploring:
				os << "not_proven_exploring";
				return os;
			case not_proven_explored:
				os << "not_proven_explored";
				return os;
			default:
				os << "bad state";
				return os;
		}
	}

	std::ostream& operator << (std::ostream& os, const hypothesis& h)
	{
		os << h.f << " --> " << h.state;
		return os;
	}

	node_id proof_tree::insert_node(const node& n)
	{
		node_id current = node_id_generator++;
		assert(current < node_id_generator); 

		bm_node.insert(bm_node_type::value_type(current, n));

		return current;
	}

	node& proof_tree::get_node(node_id n)
	{
		bm_node_type::iterator it = bm_node.find(n);
		assert(it != bm_node.end());

		return it->second;
	}

	const node& proof_tree::get_node(node_id n) const
	{
		bm_node_type::const_iterator it = bm_node.find(n);
		assert(it != bm_node.end());

		return it->second;
	}

	const hypothesis& proof_tree::get_hypothesis(hypothesis_id h ) const
	{
		bm_hyp_type::left_const_iterator it = bm_hyp.left.find(h);
		assert(it != bm_hyp.left.end());

		return it->second;
	}

	hypothesis_id proof_tree::add_hypothesis_to_node(node& n, const function_call& f)
	{
		bm_hyp_type::right_const_iterator it= bm_hyp.right.find(hypothesis(f));
		hypothesis_id current;
		if (it == bm_hyp.right.end()) {
			current = hyp_id_generator++;
			assert(current < hyp_id_generator);

			bm_hyp.insert(bm_hyp_type::value_type(current, hypothesis(f)));
		} else 
			current = it->second;

		n.hypos.push_back(current);
		return current;
	}

	void proof_tree::compute_node_state(node& n) 
	{
		bm_hyp_type::left_iterator it;
		node::const_iterator it2;
		bool all_true = true;
		for (it2 = n.cbegin(); it2 != n.cend(); ++it2) {
			it = bm_hyp.left.find(*it2);
			assert(it != bm_hyp.left.end());

			hypothesis hyp = it->second;
			// if we already proven it is false in the current context
			// or that we are to deduce that IF A THEN A
			if (hyp.state == proven_false || hyp.state == not_proven_exploring) {
				all_true = false;
				n.state = proven_false;
				continue;
			}

			if (hyp.state == not_proven_not_explored) {
				boost::logic::tribool b = ctx.f.matches(hyp.f);
				if (!boost::logic::indeterminate(b)) {
					if (b)
						hyp.state = proven_true;
					else { 
						n.state = proven_false;
						hyp.state = proven_false;
						all_true = false;
					}

					bm_hyp_type::right_iterator it3 = bm_hyp.project_right(it);
					bm_hyp.right.replace_key(it3, hyp);
				} else
					all_true = false;
			}

			if (hyp.state == not_proven_explored)
				all_true = false;

		}

		if (all_true)
			n.state = proven_true;
	}

	struct choose_hypothesis {
		const proof_tree& t;

		choose_hypothesis(const proof_tree& t) : t(t) {}

		bool operator() (const hypothesis_id id) {
			const hypothesis& h = t.get_hypothesis(id);
			return (h.state == not_proven_not_explored);
		}
	};

	struct display_rules_hyp {
		const proof_tree& t;

		display_rules_hyp(const proof_tree& t) : t(t) {}

		void operator() (hypothesis_id id) const 
		{
			const hypothesis& h = t.get_hypothesis(id);
			std::cerr << h.f << " && ";
		}
	};

	void proof_tree::try_solve_node(node& n)
	{
		compute_node_state(n);
	
		if (n.state != not_proven_not_explored)
			return;

		// choose a non explored hypothesis and explore it
		node::const_iterator it;
		do {
			n.state = not_proven_exploring;

			it = std::find_if(n.cbegin(), n.cend(), choose_hypothesis(*this));
			if (it == n.cend()) {
				n.state = not_proven_explored;
			} else {
				explore_hypothesis(*it);
			}
		} while (it != n.cend());

		// recompute the node state now that we explore all hypothesis
		compute_node_state(n);

		// if we still don't have any conclusion, just mark the node as not_proven
		if (n.state == not_proven_not_explored)
			n.state = not_proven_explored;
	}

	struct found_solution {};

	struct generate_node {
		facts_ctx& ctx;
		proof_tree &t;
		const function_call& f;
		std::vector<std::pair<rule::identifier_type, node> >& v;

		generate_node(facts_ctx& ctx, proof_tree& t, const function_call& f,
					  std::vector<std::pair<rule::identifier_type, node> >& v):
			ctx(ctx), t(t), f(f), v(v)
		{}

		void operator() (const rule& r) const 
		{
			std::vector<unifyM> unify_vect;

			std::for_each(r.action.begin(), r.action.end(),
						  apply_goal_unification(f, unify_vect));

			if (unify_vect.empty())
				return;

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
			for (size_t i = 0; i < unify_vect.size(); ++i) {
				unifyM m(unify_vect[i]);
				try {
					unifyM_candidate candidates(unbounded_symbols[i], 
												ctx.symbol_to_possible_expression[r.identifier]);
					bool has_next = true;
					while (has_next)
					{
						has_next = candidates.next(m);
						std::vector<function_call> v_f;
						std::transform(r.condition.begin(), r.condition.end(),
								std::back_inserter(v_f),
								generate_inferred_fact(m));

						node n;
						std::vector<hypothesis_id> ids(v_f.size());
						t.add_hypothesis_to_node(n, v_f.begin(), v_f.end(), ids.begin());
						t.compute_node_state(n);
						if (n.state != proven_false) 
							v.push_back(std::make_pair(r.identifier, n));

						if (n.state == proven_true) 
							throw found_solution();
					}
				} catch (no_candidate&) {}
			}
		}
	};

	struct explore_node {
		proof_tree &t;
		hypothesis &h;

		explore_node(proof_tree& t, hypothesis& h) : t(t), h(h) {}

		void operator() (std::pair<rule::identifier_type, node>& p) const
		{
			t.try_solve_node(p.second);
			node_id n_id = t.insert_node(p.second);
			h.nodes.push_back(n_id);
			if (p.second.state == proven_true)
				throw found_solution();
		}
	};

	void proof_tree::explore_hypothesis(hypothesis_id h_id)
	{
		hypothesis h = get_hypothesis(h_id);
		assert(h.state == not_proven_not_explored);
		h.state = not_proven_exploring;
		update_hypothesis(h_id, h);

		// generate all possible <rule, node> combinaison
		std::vector<std::pair<rule::identifier_type, node> > v;
		try {
			std::for_each(rs.begin(), rs.end(), generate_node(ctx, *this, h.f, v));
		} catch (const found_solution& ) {
			// the solution is the last one in the vector
			node_id n_id = insert_node(v.back().second);
			h.nodes.push_back(n_id);
			h.state = proven_true;
			update_hypothesis(h_id, h);
			return;
		}

		try {
			std::for_each(v.begin(), v.end(), explore_node(*this, h));
		} catch(const found_solution& ) {
			h.state = proven_true;
			update_hypothesis(h_id, h);
			return;
		}

		// no direct solution found ...
		h.state = not_proven_explored;
		update_hypothesis(h_id, h);
	}

	void proof_tree::update_hypothesis(hypothesis_id id, const hypothesis& h)
	{
		bm_hyp_type::left_iterator it;
		it = bm_hyp.left.find(id);
		assert(it != bm_hyp.left.end());

		bm_hyp_type::right_iterator it2 = bm_hyp.project_right(it);
		bm_hyp.right.replace_key(it2, h);
	}

	boost::logic::tribool proof_tree::compute(const function_call& f)
	{
		node n;
		add_hypothesis_to_node(n, f);
		try_solve_node(n);

		//print_node(n, 0);

		switch(n.state) {
			case proven_true:
				return true;
			case proven_false:
				return false;
			case not_proven_explored:
				return boost::logic::indeterminate;
			default:
				assert(false);
				return boost::logic::indeterminate;
		}
	}


	void proof_tree::print_node(const node& n, int indent) const
	{
		std::string ws(hyper::compiler::times(indent, " "));
		std::cerr << ws << "- Node state : " << n.state << std::endl;
		std::cerr << ws;
		std::for_each(n.cbegin(), n.cend(), display_rules_hyp(*this));
		std::cerr << "\n";

		std::for_each(n.cbegin(), n.cend(), 
					 phx::bind(&proof_tree::print_hyp, this, phx::arg_names::arg1, indent));

	}

	void proof_tree::print_node(node_id id, int n) const
	{
		return print_node(get_node(id), n);
	}

	void proof_tree::print_hyp(hypothesis_id h, int n) const 
	{
		std::string ws(hyper::compiler::times(n, " "));
		const hypothesis& hyp = get_hypothesis(h);

		void (proof_tree::*f) (node_id, int) const = &proof_tree::print_node;

		std::cerr << ws << " --> Hyp : " << hyp.f << " : " << hyp.state << std::endl;
		std::for_each(hyp.nodes.begin(), hyp.nodes.end(), 
						phx::bind(f, this, phx::arg_names::arg1, n+2));

	}

	void proof_tree::print() const
	{
		std::cerr << "Explored " << bm_hyp.size() << " hypothesis\n"; 
		bm_hyp_type::left_const_iterator it;
		for (it = bm_hyp.left.begin(); it != bm_hyp.left.end(); ++it) 
			std::cerr << it->second << "\n";
	}

	}

	bool backward_chaining::infer(const function_call& f)
	{
		details::proof_tree tree(ctx, rs);
		boost::logic::tribool b = tree.compute(f);
		return b;
	}
}}
