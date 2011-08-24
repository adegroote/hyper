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
}

namespace hyper {
	namespace compiler {
		bool recipe_condition::is_valid(const ability& a, const universe& u) const
		{
			return boost::apply_visitor(valid_helper(a, u), expr);
		}
	}
}
