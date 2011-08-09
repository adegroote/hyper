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

			dump_recipe_expression(std::ostream& oss_, const universe& u_,
								   const ability & a_, const task& t_,
								   const symbolList& syms_) : 
				oss(oss_), u(u_), a(a_), t(t_), syms(syms_)
				{}

			void operator() (const recipe_expression& r) const;
		};

		std::string exported_symbol(const std::string& s, const task& t);

		std::string mangle(const std::string& s);
	}
}

#endif /* HYPER_COMPILER_RECIPE_EXPRESSION_OUTPUT_HH_ */
