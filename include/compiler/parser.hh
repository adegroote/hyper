
#ifndef _PARSER_HH_
#define _PARSER_HH_

#include <compiler/universe.hh>

namespace hyper {
	namespace compiler {
		class parser {
			private:
				universe &u;

			public:
				parser(universe & u_) : u(u_) {};
				bool parse_ability_file(const std::string&);
		};
	};
};

#endif /* _PARSER_HH_ */
