#ifndef _LOGIC_EXPRESSION_AST_
#define _LOGIC_EXPRESSION_AST_

#include <logic/function_def.hh>

#include <vector>

#include <boost/variant.hpp>

namespace hyper {
	namespace logic {

		template <typename T>
		struct Constant
		{
			T value;
			Constant() {};
			Constant(T value_) : value(value_) {};
		};

		template <typename T>
		std::ostream& operator << (std::ostream& oss, const Constant<T>& c)
		{
			oss << c.value;
			return oss;
		}

		struct empty {};
		struct function_call;

		struct expression
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
				>
				type;

			expression()
				: expr(empty()) {}

			template <typename Expr>
			expression(Expr const& expr)
				: expr(expr) {}

			type expr;

		};

		std::ostream& operator << (std::ostream& oss, const expression& e);

			/* Will extend it with error case if needed */
		struct generate_return {
			bool res;
			expression e;
		};

		generate_return generate(const std::string&, const funcDefList&);

		bool operator == (const expression& e1, const expression& e2);

		bool operator != (const expression& e1, const expression& e2)
		{
			return ! (e1 == e2);
		}

		struct function_call {
			std::string name; // only for parsing stuff
			functionId id;
			std::vector< expression > args;
		};

		std::ostream& operator << (std::ostream& oss, const function_call& f);


	}
}

#endif /* _LOGIC_EXPRESSION_AST_ */
