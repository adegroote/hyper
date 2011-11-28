#ifndef HYPER_COMPILER_DEPENDS_HH_
#define HYPER_COMPILER_DEPENDS_HH_

#include <set>
#include <string>

namespace hyper {
	namespace compiler {
		struct expression_ast;
		struct recipe_condition;
		struct recipe_expression;
		class universe;
		struct symbolList;

		struct depends {
			std::set<std::string> fun_depends;
			std::set<std::string> var_depends;
			std::set<std::string> type_depends;
			std::set<std::string> tag_depends;
		};

		void add_depends(const expression_ast&, const std::string& context, 
						 const universe& u, depends&);

		void add_depends(const recipe_condition&, const std::string& context, 
						 const universe& u, depends& );

		void add_depends(const recipe_expression&, const std::string& context,
						 const universe& u, depends&,
						 const symbolList& s);
	}
}

#endif /* HYPER_COMPILER_DEPENDS_HH_ */
