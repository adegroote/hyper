
#ifndef _HYPER_EXPRESSION_HH_
#define _HYPER_EXPRESSION_HH_

#include <vector>

/*
 * XXX Whoot crazy not so well documented stuff
 * Increase the max size of possible variant
 * Don't use preprocessed header as they are compiled with default BOOST_MPL_LIMIT_LIST_SIZE
 */
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 30
#include <boost/variant.hpp>

namespace hyper {
	namespace compiler {
		template <typename T>
		struct Constant
		{
			T value;
		};

		struct empty {};
		struct function_call;

		enum binary_op_kind { ADD, SUB, MUL, DIV , EQ, NEQ, LT, LTE, GT, GTE, AND, OR };

		template <binary_op_kind T> 
		struct binary_op;

		std::ostream& operator << (std::ostream& os, const binary_op_kind& k);

		enum unary_op_kind { PLUS, NEG };
		
		template<unary_op_kind T>
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
				, boost::recursive_wrapper<binary_op<ADD> >
				, boost::recursive_wrapper<binary_op<SUB> >
				, boost::recursive_wrapper<binary_op<MUL> >
				, boost::recursive_wrapper<binary_op<DIV> >
				, boost::recursive_wrapper<binary_op<EQ> >
				, boost::recursive_wrapper<binary_op<NEQ> >
				, boost::recursive_wrapper<binary_op<LT> >
				, boost::recursive_wrapper<binary_op<LTE> >
				, boost::recursive_wrapper<binary_op<GT> >
				, boost::recursive_wrapper<binary_op<GTE> >
				, boost::recursive_wrapper<binary_op<AND> >
				, boost::recursive_wrapper<binary_op<OR> >
				, boost::recursive_wrapper<unary_op<NEG> >
				>
				type;

			expression_ast()
				: expr(empty()) {}

			template <typename Expr>
				expression_ast(Expr const& expr)
				: expr(expr) {}

			type expr;

			template <binary_op_kind T>
			expression_ast& binary(const expression_ast& right)
			{
				expr = binary_op<T>(expr, right);
				return *this;
			}


			template <unary_op_kind T>
			expression_ast& unary(const expression_ast& subject)
			{
				expr = unary_op<T>(subject);
				return *this;
			}
		};

		std::ostream& operator << (std::ostream& os, const expression_ast& e);

			
		template <binary_op_kind T>
		struct binary_op
		{
			binary_op(
					  expression_ast const& left
					, expression_ast const& right)
				: op(T), left(left), right(right) {}

			binary_op_kind op;
			expression_ast left;
			expression_ast right;
		};


		template <unary_op_kind T>
		struct unary_op
		{
			unary_op(expression_ast const& subject)
				: op(T), subject(subject) {}

			unary_op_kind op;
			expression_ast subject;
		};

		struct function_call {
			std::string fName;
			std::vector< expression_ast > args;
		};
	};
};



#endif /* _HYPER_EXPRESSION_HH_ */
