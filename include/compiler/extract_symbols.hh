#ifndef HYPER_COMPILER_EXTRACT_SYMBOLS_HH_
#define HYPER_COMPILER_EXTRACT_SYMBOLS_HH_

#include <string>
#include <set>

namespace hyper {
	namespace compiler {

		struct expression_ast;

		struct extract_symbols {
			std::set<std::string> local;
			std::set<std::string> remote;
			std::string context;

			extract_symbols(const std::string& context);
			void extract(const expression_ast& e);
		};
	}
}

#endif /* HYPER_COMPILER_EXTRACT_SYMBOL_HH_ */
