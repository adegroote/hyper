#ifndef HYPER_COMPILER_DEPENDS_HH_
#define HYPER_COMPILER_DEPENDS_HH_

#include <set>
#include <string>

namespace hyper {
	namespace compiler {
		struct expression_ast;
		struct recipe_expression;

		struct depends {
			std::set<std::string> fun_depends;
			std::set<std::string> var_depends;
		};

		void add_depends(const expression_ast&, const std::string& context, depends& );

		void add_depends(const recipe_expression&, const std::string& context, depends& );
	}
}

#endif /* HYPER_COMPILER_DEPENDS_HH_ */
