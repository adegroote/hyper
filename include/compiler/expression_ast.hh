
#ifndef _HYPER_EXPRESSION_HH_
#define _HYPER_EXPRESSION_HH_

#include <vector>

#include <boost/mpl/contains.hpp>
#include <boost/variant.hpp>
#include <boost/optional/optional_fwd.hpp>
#include <boost/utility/enable_if.hpp>

namespace hyper {
	namespace compiler {

		class ability;
		class universe;
		class symbolList;
		struct recipe_expression;

		template <typename T>
		struct Constant
		{
			T value;
			Constant() {};
			Constant(T value_) : value(value_) {};
		};

		struct empty {};
		struct function_call;

		enum binary_op_kind { ADD, SUB, MUL, DIV , EQ, NEQ, LT, LTE, GT, GTE, AND, OR };

		struct binary_op;

		std::ostream& operator << (std::ostream& os, const binary_op_kind& k);

		enum unary_op_kind { PLUS, NEG };
		
		struct unary_op;

		std::ostream& operator << (std::ostream& os, const unary_op_kind& k);

		struct expression_ast
		{
			typedef
				boost::variant<
				  empty								// can't happen
				, Constant<int>						// constant of type int
				, Constant<double>					// constant of type double
				, Constant<std::string>				// constant of type string
				, Constant<bool>					// constant of type bool
				, std::string						// variable identifier
				, boost::recursive_wrapper<function_call>
				, boost::recursive_wrapper<expression_ast>
				, boost::recursive_wrapper<binary_op>
				, boost::recursive_wrapper<unary_op>
				>
				type;

			expression_ast()
				: expr(empty()) {}

			/* type::types contains the sequence of unwrapped types of the
			 * variant, not sure it is really part of the interface */
			template <typename Expr>
			expression_ast(const Expr& expr,
						   typename boost::enable_if<boost::mpl::contains<type::types, Expr> >::type* = 0) 
				: expr(expr) {}

			expression_ast(type expr) : expr(expr) {}

			type expr;

			expression_ast& binary(binary_op_kind k, const expression_ast& right);


			expression_ast& unary(unary_op_kind k, const expression_ast& subject);

			void reduce();

			bool is_valid(const ability&, const universe&,
						 const boost::optional<symbolList>& s) const;

			bool is_valid_predicate(const ability&, const universe&,
									const boost::optional<symbolList>& s) const;
		};

		std::ostream& operator << (std::ostream& os, const expression_ast& e);

			
		struct binary_op
		{
			binary_op(
					  binary_op_kind k,
					  expression_ast const& left
					, expression_ast const& right)
				: op(k), left(left), right(right) {}

			binary_op_kind op;
			expression_ast left;
			expression_ast right;
		};


		struct unary_op
		{
			unary_op(unary_op_kind k, expression_ast const& subject)
				: op(k), subject(subject) {}

			unary_op_kind op;
			expression_ast subject;
		};

		inline
		expression_ast& expression_ast::binary(binary_op_kind k, const expression_ast& right)
		{
			expr = binary_op(k, expr, right);
			return *this;
		}

		inline
		expression_ast& expression_ast::unary(unary_op_kind k, const expression_ast& subject)
		{
			expr = unary_op(k, subject);
			return *this;
		}

		struct function_call {
			std::string fName;
			std::vector< expression_ast > args;
		};
	}
}



#endif /* _HYPER_EXPRESSION_HH_ */
