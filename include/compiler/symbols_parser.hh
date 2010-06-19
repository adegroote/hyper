
#ifndef _SYMBOLS_PARSER_HH_
#define _SYMBOLS_PARSER_HH_

#include <iostream>
#include <string>
#include <vector>

namespace hyper {
	namespace compiler {
		/* Structs used by parser */

		struct symbol_decl {
			std::string typeName;
			std::string name;
		};

		std::ostream& operator << (std::ostream& os, const symbol_decl&);

		struct symbol_decl_list {
			std::vector < symbol_decl> l;
		};

		std::ostream& operator << (std::ostream& os, const symbol_decl_list&);
	}
}

#endif /* _SYMBOLS_PARSER_HH_ */
