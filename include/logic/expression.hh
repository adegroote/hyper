#ifndef _LOGIC_EXPRESSION_AST_
#define _LOGIC_EXPRESSION_AST_

#include <logic/function_def.hh>

#include <vector>

#include <boost/variant/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

#include <boost/preprocessor/repetition/repeat.hpp>

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
		bool operator < (const Constant<T>& t1, const Constant<T>& t2)
		{
			return t1.value < t2.value;
		}

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

		bool operator == (const expression& e1, const expression& e2);

		inline bool operator != (const expression& e1, const expression& e2)
		{
			return ! (e1 == e2);
		}

		bool operator < (const expression& e1, const expression& e2);

		
		boost::optional<expression> generate_node(const std::string&);

#define NEW_PARAMS_ARGS(z, n, _) ,const expression& arg ## n
#define PUSH_BACK_ARGS(z, n, _) args.push_back(arg ## n);
#define EVAL_MAX_ARGS 10

#define NEW_CTR_DECL(z, n, _) \
	function_call(const std::string& name BOOST_PP_REPEAT(n, NEW_PARAMS_ARGS, _) )\
				  : name(name) {\
			BOOST_PP_REPEAT(n, PUSH_BACK_ARGS, _) \
		}

		struct function_call {
			std::string name; // only for parsing stuff
			functionId id;
			std::vector< expression > args;

			function_call() {}

			BOOST_PP_REPEAT(BOOST_PP_INC(EVAL_MAX_ARGS), NEW_CTR_DECL, _)
		};

#undef EVAL_MAX_ARGS
#undef PUSH_BACK_ARGS
#undef NEW_PARAMS_ARGS

		bool operator < (const function_call& f1, const function_call& f2);

		/* Will extend it with error case if needed */
		struct generate_return {
			bool res;
			function_call e;
		};

		generate_return generate(const std::string&, const funcDefList&);

		/* Just fill the functionId of each function_call, as it is not the
		 * same between different agents*/
		generate_return generate(const function_call& f, const funcDefList&);

		std::ostream& operator << (std::ostream& oss, const function_call& f);
	}
}

#endif /* _LOGIC_EXPRESSION_AST_ */
