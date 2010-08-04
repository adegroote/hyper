#include <logic/engine.hh>
#include <logic/unify.hh>

#include <boost/bind.hpp>

#include <set>

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
}

namespace hyper {
	namespace logic {
		engine::engine() : facts_(funcs_), rules_(funcs_) {}

		bool engine::add_func(const std::string& name, size_t arity,
							  eval_predicate* eval)
		{
			funcs_.add(name, arity, eval);
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

			bool match = facts_.matches(f);
			if (match)
				return match;

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
