
#ifndef _PARSER_HH_
#define _PARSER_HH_

#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include <compiler/universe.hh>

namespace hyper {
	namespace compiler {
		class parser {
			private:
				universe &u;
				std::vector < boost::filesystem::path > parsed_files;

			public:
				parser(universe & u_) : u(u_) {};
				bool parse_ability_file(const std::string&);
				bool parse_expression(const std::string&);
				bool parse_task(const std::string&);
				bool parse_task_file(const std::string&);
		};
	}
}

#endif /* _PARSER_HH_ */
