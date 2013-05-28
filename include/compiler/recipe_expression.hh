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
		struct while_decl;
		struct ensure_decl;

		typedef std::pair<expression_ast, expression_ast> unification_expression;

		struct logic_expression_decl
		{
			expression_ast main;
			std::vector<unification_expression> unification_clauses;
		};

		std::ostream& operator << (std::ostream&, const unification_expression&);
		std::ostream& operator << (std::ostream&, const logic_expression_decl&);

		struct abort_decl {
			std::string identifier;

			abort_decl() {}
			abort_decl(const std::string& identifier_) :
				identifier(identifier_) {}
		};

		std::ostream& operator<< (std::ostream&, const abort_decl&);
		
		enum observer_op_kind { WAIT, ASSERT };

		std::ostream& operator<< (std::ostream& os, observer_op_kind);

		template <observer_op_kind kind> 
		struct observer_op {
			expression_ast content;
			boost::optional<double> delay;

			observer_op() {}
			observer_op(const expression_ast& content, boost::optional<double> delay) : 
				content(content), delay(delay) {}
		};

		template <observer_op_kind kind>
		std::ostream& operator << (std::ostream& os, const observer_op<kind>& op)
		{
			os << kind << "(" << op.content << ")";
			return os;
		}

		enum recipe_op_kind { MAKE, ENSURE };

		std::ostream& operator<< (std::ostream& os, recipe_op_kind kind);

		struct remote_constraint {
			boost::optional<std::string> dst;
			logic_expression_decl logic_expr;

			remote_constraint() {}
			remote_constraint(const expression_ast& ast);
			remote_constraint(const logic_expression_decl& decl);

			void compute_dst();
		};

		std::ostream& operator<< (std::ostream& os, const remote_constraint&);

		template <recipe_op_kind kind>
		struct recipe_op 
		{
			std::vector<remote_constraint> content;
			boost::optional<double> delay;

			recipe_op() {}
			recipe_op(const std::vector<expression_ast>& content_) :
				delay(boost::none)
			{
				std::copy(content_.begin(), content_.end(),
						  std::back_inserter(content));
			}
			
			recipe_op(const std::vector<logic_expression_decl>& content_) :
				delay(boost::none)
			{
				std::copy(content_.begin(), content_.end(),
						  std::back_inserter(content));
			}
		};

		template <recipe_op_kind kind>
		std::ostream& operator<< (std::ostream& os, const recipe_op<kind>& r)
		{
			os << kind << " ("; 
			std::copy(r.content.begin(), r.content.end(), 
					  std::ostream_iterator<remote_constraint>(os, "\n"));
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
				boost::recursive_wrapper<while_decl>,
				abort_decl, 
				set_decl,
				observer_op<WAIT>,
				observer_op<ASSERT>,
				recipe_op<MAKE>,
				recipe_op<ENSURE>,
				expression_ast
			>
			type;

			type expr;

			recipe_expression() : expr(empty()) {}

			template <typename Expr>
			recipe_expression(const Expr& expr,
							  typename boost::enable_if<boost::mpl::contains<type::types, Expr> >::type* = 0) 
				: expr(expr) {}

			recipe_expression(const type& expr) : expr(expr) {}

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

		struct while_decl {
			expression_ast condition;
			std::vector<recipe_expression> body;

			while_decl() {}
			while_decl(const expression_ast& condition, const std::vector<recipe_expression>& body):
				condition(condition), body(body)
			{}
		};

		std::ostream& operator<< (std::ostream&, const while_decl&);


	}
}
#endif /* HYPER_COMPILER_RECIPE_EXPRESSION_HH_ */
