#ifndef HYPER_COMPILER_RECIPE_EXPRESSION_HH_
#define HYPER_COMPILER_RECIPE_EXPRESSION_HH_

#include <iostream>
#include <string>

#include <boost/variant.hpp>
#include <compiler/expression_ast.hh>

namespace hyper {
	namespace compiler {
		struct let_decl;

		struct abort_decl {
			std::string identifier;

			abort_decl() {}
			abort_decl(const std::string& identifier_) :
				identifier(identifier_) {}
		};

		std::ostream& operator<< (std::ostream&, const abort_decl&);

		enum recipe_op_kind { MAKE, ENSURE, WAIT };

		std::ostream& operator<< (std::ostream& os, recipe_op_kind kind);

		template <recipe_op_kind kind>
		struct recipe_op 
		{
			expression_ast content;

			recipe_op() {}
			recipe_op(const expression_ast& content_) : content(content_) {}
		};

		template <recipe_op_kind kind>
		std::ostream& operator<< (std::ostream& os, const recipe_op<kind>& r)
		{
			os << kind << " " << r.content;
			return os;
		}

		struct recipe_expression {
			typedef boost::variant<
				empty,
				boost::recursive_wrapper<let_decl>,
				abort_decl, 
				recipe_op<MAKE>,
				recipe_op<ENSURE>,
				recipe_op<WAIT>,
				expression_ast
			>
			type;

			type expr;

			recipe_expression() : expr(empty()) {}

			template <typename Expr>
				recipe_expression(Expr const& expr)
				: expr(expr) {}

		};

		std::ostream& operator<< (std::ostream& os, const recipe_expression&);

		struct let_decl {
			std::string identifier;
			recipe_expression bounded;

			let_decl() {}
			let_decl(const std::string& identifier_, recipe_expression bounded_) :
				identifier(identifier_), bounded(bounded_) {}
		};

		std::ostream& operator<< (std::ostream&, const let_decl&);
	}
}
#endif /* HYPER_COMPILER_RECIPE_EXPRESSION_HH_ */
