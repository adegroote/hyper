#include <compiler/ability.hh>
#include <compiler/recipe_condition.hh>
#include <compiler/universe.hh>

namespace {
	using namespace hyper::compiler;

	struct valid_helper : public boost::static_visitor<bool>
	{
		const ability& a;
		const universe& u;

		valid_helper(const ability& a, const universe& u) : a(a), u(u) {}

		template <typename T>
		bool operator() (const T& t) const { return false; }

		bool operator() (const expression_ast& ast) const {
			return ast.is_valid_predicate(a, u, boost::none);
		}
	};


	struct recipe_condition_print : public boost::static_visitor<void>
	{
		std::ostream& os;

		recipe_condition_print(std::ostream& os_) : os(os_) {}

		template <typename T>
		void operator() (const T& t) const
		{
			os << t;
		}

		void operator() (const empty&) const {}
	};
}

namespace hyper {
	namespace compiler {
		bool recipe_condition::is_valid(const ability& a, const universe& u) const
		{
			return boost::apply_visitor(valid_helper(a, u), expr);
		}

		std::ostream& operator<< (std::ostream& os, const recipe_condition& cond)
		{
			boost::apply_visitor(recipe_condition_print(os), cond.expr);
			return os;
		}
	}
}
