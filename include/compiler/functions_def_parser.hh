#ifndef _FUNCTIONS_DEF_PARSER_HH_
#define _FUNCTIONS_DEF_PARSER_HH_

#include <iostream>
#include <string>
#include <vector>

namespace hyper {
	namespace compiler {
		struct function_decl {
			std::string fName;
			std::string returnName;
			std::vector < std::string > argsName;
		};

		std::ostream& operator << (std::ostream& os, const function_decl& decl);

		struct function_decl_list {
			std::vector < function_decl > l;
		};

		std::ostream& operator << (std::ostream& os, const function_decl_list& l);
	};
};

#endif /* _FUNCTIONS_DEF_PARSER_HH_ */
