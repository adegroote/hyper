#ifndef HYPER_COMPILER_RECIPE_CONDITION_HH_
#define HYPER_COMPILER_RECIPE_CONDITION_HH_

#include <iostream>
#include <boost/variant.hpp>

#include <compiler/expression_ast.hh>

namespace hyper {
	namespace compiler {
		class ability;
		class universe;

		struct recipe_condition {
			typedef boost::variant<
				empty,
				expression_ast
			>
			type;

			type expr;

			recipe_condition() : expr(empty()) {}

			template <typename Expr>
			recipe_condition(const Expr& expr,
							  typename boost::enable_if<boost::mpl::contains<type::types, Expr> >::type* dummy = 0) 
				: expr(expr) {}

			recipe_condition(const type& expr) : expr(expr) {}

			bool is_valid(const ability&, const universe&) const;
		};

		std::ostream& operator<< (std::ostream&, const recipe_condition&);
	}
}

#endif /* HYPER_COMPILER_RECIPE_CONDITION_HH_ */
