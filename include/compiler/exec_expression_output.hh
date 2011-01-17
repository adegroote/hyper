#ifndef HYPER_COMPILER_EXEC_EXPRESSION_OUTPUT_HH_
#define HYPER_COMPILER_EXEC_EXPRESSION_OUTPUT_HH_

#include <iostream>
#include <string>

#include <boost/optional.hpp>

namespace hyper {
	namespace compiler {
		struct universe;
		struct ability;
		struct task;
		struct typeList;
		struct recipe;


		struct exec_expression_output
		{
			const ability& ability_context;
			const std::string context_name;
			std::ostream& oss;
			size_t counter;
			const std::string base_expr;
			const std::set<std::string> remote_symbols;

			exec_expression_output(
					const ability& ability_context_, 
					const std::string& context_name,
					std::ostream& oss_,
					const std::string& base_expr_,
					const std::set<std::string> remote_symbols):
				ability_context(ability_context_),
				context_name(context_name),
				oss(oss_), 
				counter(0),
				base_expr(base_expr_),
				remote_symbols(remote_symbols)
			{}

			void operator() (const expression_ast& e);
		};

		std::string expression_ast_output(const expression_ast& e);
	}
}

#endif /* HYPER_COMPILER_EXEC_EXPRESSION_OUTPUT_HH_ */
