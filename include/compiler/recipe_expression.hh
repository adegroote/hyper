#ifndef HYPER_COMPILER_RECIPE_EXPRESSION_HH_
#define HYPER_COMPILER_RECIPE_EXPRESSION_HH_

#include <iostream>
#include <string>

#include <boost/optional.hpp>
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
		
		struct wait_decl {
			expression_ast content;

			wait_decl() {}
			wait_decl(const expression_ast& content) : content(content) {}
		};

		std::ostream& operator << (std::ostream& os, const wait_decl&);

		enum recipe_op_kind { MAKE, ENSURE };

		std::ostream& operator<< (std::ostream& os, recipe_op_kind kind);

		struct remote_constraint {
			boost::optional<std::string> dst;
			expression_ast ast;

			remote_constraint() {}
			remote_constraint(const expression_ast& ast);
		};

		std::ostream& operator<< (std::ostream& os, const remote_constraint&);

		template <recipe_op_kind kind>
		struct recipe_op 
		{
			std::vector<remote_constraint> content;

			recipe_op() {}
			recipe_op(const std::vector<expression_ast>& content_) {
				std::copy(content_.begin(), content_.end(),
						  std::back_inserter(content));
			}
		};

		template <recipe_op_kind kind>
		std::ostream& operator<< (std::ostream& os, const recipe_op<kind>& r)
		{
			os << kind << " ("; 
			std::copy(r.content.begin(), r.content.end(), 
					  std::ostream_iterator<expression_ast>(os, "\n"));
			os << " )";
			return os;
		}

		struct set_decl {
			std::string identifier;
			expression_ast bounded;

			set_decl() {}
			set_decl(const std::string& identifier_, expression_ast bounded_) :
				identifier(identifier_), bounded(bounded_) {}
		};

		std::ostream& operator<< (std::ostream&, const set_decl&);

		struct recipe_expression {
			typedef boost::variant<
				empty,
				boost::recursive_wrapper<let_decl>,
				abort_decl, 
				set_decl,
				wait_decl,
				recipe_op<MAKE>,
				recipe_op<ENSURE>,
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
