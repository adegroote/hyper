#ifndef HYPER_COMPILER_DEPENDS_HH_
#define HYPER_COMPILER_DEPENDS_HH_

#include <set>
#include <string>

namespace hyper {
	namespace compiler {
		struct expression_ast;
		struct typeList;

		void add_depends(const expression_ast&, const std::string& context, 
						 const typeList&, std::set<std::string>& );
	};
};

#endif /* HYPER_COMPILER_DEPENDS_HH_ */
