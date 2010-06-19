
#ifndef _ABILITY_PARSER_HH_
#define _ABILITY_PARSER_HH_

#include <iostream>
#include <string>

#include <compiler/types_parser.hh>
#include <compiler/symbols_parser.hh>
#include <compiler/functions_def_parser.hh>

namespace hyper {
	namespace compiler {
		struct programming_decl {
			type_decl_list types;
			function_decl_list funcs;
		};

		std::ostream& operator << (std::ostream&, const programming_decl& p);

		struct ability_blocks_decl {
			symbol_decl_list controlables;
			symbol_decl_list readables;
			symbol_decl_list privates;

			programming_decl env;
		};

		std::ostream& operator << (std::ostream&, const ability_blocks_decl& d);

		struct ability_decl {
			std::string name;

			ability_blocks_decl blocks;
		};

		std::ostream& operator << (std::ostream&, const ability_decl& a);
	}
}

#endif /* _ABILITY_PARSER_HH_ */
