#include <logic/engine.hh>

#include <boost/bind.hpp>

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
			return true;
		}

		bool engine::add_fact(const std::vector<std::string>& exprs)
		{
			// help the compiler to choose right overload
			bool (facts::*f) (const std::string& s) = &facts::add;
			std::for_each(exprs.begin(), exprs.end(), 
						  boost::bind(f, boost::ref(facts_), _1));
			return true;
		}

		bool engine::add_rule(const std::string& identifier,
							  const std::vector<std::string>& cond,
							  const std::vector<std::string>& action)
		{
			return rules_.add(identifier, cond, action);
		}

		boost::logic::tribool engine::infer(const std::string& goal)
		{
			generate_return r = generate(goal, funcs_);
			assert(r.res);

			return boost::logic::indeterminate;
		}
	}
}
