#ifndef HYPER_COMPILER_EXTRACT_SYMBOLS_HH_
#define HYPER_COMPILER_EXTRACT_SYMBOLS_HH_

#include <string>
#include <set>

namespace hyper {
	namespace compiler {

		struct expression_ast;
		struct recipe_condition;
		struct universe;
		struct ability;

		struct extract_symbols {
			std::set<std::string> local;
			std::set<std::string> local_with_updater;
			std::set<std::string> remote;
			const ability& a;

			extract_symbols(const ability &a);
			void extract(const expression_ast& e);
			void extract(const recipe_condition& cond);

			std::string 
			remote_vector_type_output(const universe& u) const;
			std::string 
			remote_list_variables(const std::string& base_indent) const;
			std::string 
			local_list_variables(const std::string& base_indent) const;
			std::string
			local_list_variables_updated(const std::string& base_indent) const;
			bool empty() const { return local_with_updater.empty() && remote.empty(); }
							
		};
	}
}

#endif /* HYPER_COMPILER_EXTRACT_SYMBOL_HH_ */
