#ifndef _RULES_DEF_PARSER_HH_
#define _RULES_DEF_PARSER_HH_

#include <iostream>
#include <string>
#include <vector>

#include <compiler/expression_ast.hh>


namespace hyper {
	namespace compiler {

		class universe;
		class ability;

		struct rule_decl {
			std::string name;
			/* Can be empty */
			std::vector<expression_ast> premises;
			std::vector<expression_ast> conclusions;
		};

		std::ostream& operator << (std::ostream& os, const rule_decl& decl);

		struct rule_decl_list {
			std::vector <rule_decl> l;
		};

		std::ostream& operator << (std::ostream& os, const rule_decl_list& l);
	}
}

#endif /* _RULES_DEF_PARSER_HH_ */
