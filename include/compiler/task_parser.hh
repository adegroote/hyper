
#ifndef _COMPILER_TASK_PARSER_HH_
#define _COMPILER_TASK_PARSER_HH_

#include <iostream>
#include <string>
#include <vector>

#include <compiler/expression_ast.hh>

namespace hyper {
	namespace compiler {

		struct cond_list_decl {
			std::vector<expression_ast> list;
		};

		std::ostream& operator << (std::ostream&, const cond_list_decl & l);

		struct task_decl {
			std::string name;
			cond_list_decl pre;
			cond_list_decl post;
		};

		std::ostream& operator << (std::ostream&, const task_decl& t);

		struct task_decl_list {
			std::vector<task_decl> list;
		};

		std::ostream& operator << (std::ostream&, const task_decl_list& l);

		struct task_decl_list_context {
			std::string ability_name;
			task_decl_list list;
		};

		std::ostream& operator << (std::ostream&, const task_decl_list_context & l);

	}
}

#endif /* _COMPILER_TASK_PARSER_HH_ */
