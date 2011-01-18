#ifndef HYPER_COMPILER_EXTRACT_SYMBOLS_HH_
#define HYPER_COMPILER_EXTRACT_SYMBOLS_HH_

#include <string>
#include <set>

namespace hyper {
	namespace compiler {

		struct expression_ast;
		struct universe;

		struct extract_symbols {
			std::set<std::string> local;
			std::set<std::string> remote;
			std::string context;

			extract_symbols(const std::string& context);
			void extract(const expression_ast& e);

			std::string remote_vector_type_output(const universe& u) const;
			std::string remote_list_variables(const std::string& base_indent) const;
			std::string local_list_variables(const std::string& base_indent) const;
		};
	}
}

#endif /* HYPER_COMPILER_EXTRACT_SYMBOL_HH_ */
