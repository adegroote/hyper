#ifndef HYPER_COMPILER_RECIPE_CONTEXT_HH_
#define HYPER_COMPILER_RECIPE_CONTEXT_HH_

#include <map>
#include <string>
#include <vector>

#include <compiler/recipe_parser.hh>
#include <compiler/recipe_condition.hh>

namespace hyper {
	namespace compiler {
		struct recipe_fun_def {
			std::string name;
			std::vector<std::string> args;
			std::vector<recipe_expression> impl;
			std::vector<std::string> local_vars;

			recipe_fun_def();
			recipe_fun_def(const fun_decl&);
		};

		typedef std::map<std::string, recipe_fun_def> map_fun_def;
		map_fun_def make_map_fun_def(const recipe_context_decl& decl);

		void apply_fun_body(std::vector<recipe_expression> & body_, const map_fun_def& map);


		struct adapt_expression_to_context {
			const recipe_context_decl::map_type& map;

			adapt_expression_to_context(const recipe_context_decl::map_type& map) : 
				map(map)
			{}

			expression_ast operator() (const expression_ast& ast) const;
		};

		struct adapt_recipe_condition_to_context {
			const recipe_context_decl::map_type& map;

			adapt_recipe_condition_to_context(const recipe_context_decl::map_type& map) : 
				map(map)
			{}

			recipe_condition operator() (const recipe_condition & ast) const;
		};

		struct adapt_recipe_expression_to_context {
			const recipe_context_decl::map_type& map;

			adapt_recipe_expression_to_context(const recipe_context_decl::map_type& map) :
				map(map)
			{}

			recipe_expression operator() (const recipe_expression& ast) const;
		};
	}
}
	
#endif /* HYPER_COMPILER_RECIPE_CONTEXT_HH_ */
