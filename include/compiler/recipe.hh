#ifndef HYPER_COMPILER_RECIPE_HH_
#define HYPER_COMPILER_RECIPE_HH_

#include <iostream>
#include <string>
#include <vector>

#include <compiler/symbols.hh>
#include <compiler/expression_ast.hh>
#include <compiler/recipe_expression.hh>

namespace hyper {
	namespace compiler {

		class ability;
		class universe;
		class typeList;
		class task;
		struct recipe_decl;

		class recipe {
			private:
				std::string name;
				std::vector<expression_ast> pre;
				std::vector<expression_ast> post;
				std::vector<recipe_expression> body;

				const ability& context_a;
				const task& context_t;
				const typeList& tList;
				symbolList local_symbol;
			
			public:
				recipe(const recipe_decl&i, const ability&, const task&, const typeList&);
				/*
				 * This method is not const, because it currently build local_symbol, while
				 * verifying the coherency of the recipe
				 */
				bool validate(const universe&) ;
				void dump(std::ostream& oss, const universe&) const;

				const std::string& get_name() const { return name; }
		};
	}
}


#endif /* HYPER_COMPILER_RECIPE_HH_ */
