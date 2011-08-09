#ifndef HYPER_COMPILER_EXTRACT_UNUSED_RESULT_HH_
#define HYPER_COMPILER_EXTRACT_UNUSED_RESULT_HH_

#include <set>
#include <string>

namespace hyper {
	namespace compiler {
		class universe;
		class ability;
		class symbolList;
		struct recipe_expression;

		struct extract_unused_result {
			std::set<std::string>& list;
			const universe& u;
			const ability& a;
			const symbolList& syms;

			extract_unused_result(std::set<std::string>& list, const universe& u, 
									const ability& a, const symbolList& syms) :
				list(list), u(u), a(a), syms(syms)
			{}

			void operator() (const recipe_expression& r);
		};
	}
}

#endif /* HYPER_COMPILER_EXTRACT_UNUSED_RESULT_HH_ */
