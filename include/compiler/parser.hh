
#ifndef _PARSER_HH_
#define _PARSER_HH_

#include <compiler/types.hh>
#include <compiler/symbols.hh>
#include <compiler/functions_def.hh>

namespace hyper {
	namespace compiler {
		class parser {
			private:
				typeList tList;
				symbolList sList;
				functionDefList fList;

			public:
				parser();
				bool parse(const std::string &);
				bool parse_ability_file(const std::string&);
		};
	};
};

#endif /* _PARSER_HH_ */
