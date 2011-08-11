#ifndef HYPER_COMPILER_RECIPE_PARSER_HH_
#define HYPER_COMPILER_RECIPE_PARSER_HH_

#include <iostream>
#include <string>
#include <vector>
#include <map>

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

		struct letname_expression_decl {
			std::string name;
			expression_ast expr;
		};

		std::ostream& operator << (std::ostream&, const letname_expression_decl&);

		struct recipe_context_decl {
			typedef std::map<std::string, expression_ast> map_type;
			std::vector<letname_expression_decl> expression_names;
		};

		std::ostream& operator << (std::ostream& , const recipe_context_decl&);

		recipe_context_decl::map_type 
		make_name_expression_map(const recipe_context_decl& decl);

		struct recipe_decl_list {
			recipe_context_decl context;
			std::vector<recipe_decl> recipes;
		};

		std::ostream& operator<< (std::ostream&, const recipe_decl_list &);
	}
}

#endif /* HYPER_COMPILER_RECIPE_PARSER_HH_ */
