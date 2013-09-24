#ifndef HYPER_COMPILER_RECIPE_EXPRESSION_OUTPUT_HH_
#define HYPER_COMPILER_RECIPE_EXPRESSION_OUTPUT_HH_

#include <iostream>

namespace hyper { 
	namespace compiler {
		class universe;
		class ability;
		class task;
		class symbolList;
		struct recipe_expression;

		struct dump_recipe_expression {
			std::ostream& oss;
			const universe &u;
			const ability &a;
			const task& t;
			const symbolList& syms;
			mutable size_t counter;
			mutable size_t inner_var_counter;
			mutable size_t computation_counter;

			dump_recipe_expression(std::ostream& oss_, const universe& u_,
								   const ability & a_, const task& t_,
								   const symbolList& syms_, size_t counter = 0,
								   size_t inner_var_counter = 0,
								   size_t computation_counter = 0) : 
				oss(oss_), u(u_), a(a_), t(t_), syms(syms_), counter(counter),
				inner_var_counter(inner_var_counter),
				computation_counter(computation_counter)
				{}

			void operator() (const recipe_expression& r) const;
		};

		std::string exported_symbol(const std::string& s, const task& t);

		std::string mangle(const std::string& s);
	}
}

#endif /* HYPER_COMPILER_RECIPE_EXPRESSION_OUTPUT_HH_ */
