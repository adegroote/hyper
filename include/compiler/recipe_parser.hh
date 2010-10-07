#ifndef HYPER_COMPILER_RECIPE_PARSER_HH_
#define HYPER_COMPILER_RECIPE_PARSER_HH_

#include <iostream>
#include <string>
#include <vector>

#include <compiler/task_parser.hh>
#include <compiler/recipe_expression.hh>

namespace hyper {
	namespace compiler {
		struct body_block_decl {
			std::vector<recipe_expression> list;
		};

		std::ostream& operator<< (std::ostream&, const body_block_decl&);

		struct recipe_decl {
			std::string name;
			cond_block_decl conds;
			body_block_decl body;
		};

		std::ostream& operator<< (std::ostream&, const recipe_decl&);

		struct recipe_decl_list {
			std::vector<recipe_decl> recipes;
		};

		std::ostream& operator<< (std::ostream&, const recipe_decl_list &);
	}
}

#endif /* HYPER_COMPILER_RECIPE_PARSER_HH_ */
