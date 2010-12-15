#ifndef HYPER_COMPILER_EXEC_EXPRESSION_OUTPUT_HH_
#define HYPER_COMPILER_EXEC_EXPRESSION_OUTPUT_HH_

#include <iostream>
#include <string>

namespace hyper {
	namespace compiler {
		struct universe;
		struct ability;
		struct task;
		struct typeList;


		struct exec_expression_output
		{
			const universe& u;
			const ability& ability_context;
			const task& task_context;
			std::ostream& oss;
			const typeList& tList;
			size_t counter;
			size_t num_conds;
			const std::string base_expr;

			exec_expression_output(const universe& u_,
					const ability& ability_context_, 
					const task& task_context_,
					std::ostream& oss_,
					const typeList& tList_,
					size_t num_conds_,
					const std::string& base_expr_):
				u(u_),
				ability_context(ability_context_),
				task_context(task_context_),
				oss(oss_), 
				tList(tList_),
				counter(0),
				num_conds(num_conds_),
				base_expr(base_expr_)
			{}

			void operator() (const expression_ast& e);
		};
	}
}

#endif /* HYPER_COMPILER_EXEC_EXPRESSION_OUTPUT_HH_ */
