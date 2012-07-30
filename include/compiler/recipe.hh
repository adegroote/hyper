#ifndef HYPER_COMPILER_RECIPE_HH_
#define HYPER_COMPILER_RECIPE_HH_

#include <iostream>
#include <string>
#include <vector>

#include <compiler/symbols.hh>
#include <compiler/expression_ast.hh>
#include <compiler/recipe_condition.hh>
#include <compiler/recipe_expression.hh>

namespace hyper {
	namespace compiler {

		class ability;
		class universe;
		class typeList;
		class task;
		class depends;
		struct recipe_decl;
		struct recipe_context_decl;

		class recipe {
			private:
				std::string name;
				std::vector<recipe_condition> pre;
				std::vector<recipe_condition> post;
				std::vector<recipe_expression> body;
				std::vector<recipe_expression> end;
				unsigned int prefer;

				const ability& context_a;
				const task& context_t;
				const typeList& tList;
				symbolList local_symbol;

				/* Compute if we have pre_condition of type expression_ast 
				 * These one will have some associated generated code 
				 * */
				bool has_preconds;
			
			public:
				recipe(const recipe_decl&, const recipe_context_decl&,
					   const ability&, const task&, const typeList&);
				/*
				 * This method is not const, because it currently build local_symbol, while
				 * verifying the coherency of the recipe
				 */
				bool validate(const universe&) ;
				void dump_include(std::ostream& oss, const universe&) const;
				void dump(std::ostream& oss, const universe&) const;
				void add_depends(depends& deps, const universe& u) const;

				const std::string& get_name() const { return name; }
				std::string exported_name() const {	return "_" + context_t.get_name() + 
														   "_recipe_" + name; }
		};
	}
}


#endif /* HYPER_COMPILER_RECIPE_HH_ */
