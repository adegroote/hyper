
#ifndef _HYPER_COMPILER_TASK_HH_
#define _HYPER_COMPILER_TASK_HH_ 

#include <iostream>
#include <string>
#include <vector>

#include <compiler/expression_ast.hh>
#include <compiler/types.hh>

namespace hyper {
	namespace compiler {

		class ability;
		class universe;

		struct task {
			std::string name;
			std::vector<expression_ast> pre;
			std::vector<expression_ast> post;

			void dump(std::ostream& oss, const typeList& tList, 
					  const universe& u, const ability& context) const;
		};

		std::ostream& operator<< (std::ostream& oss, const task& t);
	}
}

#endif /* _HYPER_COMPILER_TASK_HH_ */
