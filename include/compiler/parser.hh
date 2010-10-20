
#ifndef _PARSER_HH_
#define _PARSER_HH_

#include <string>
#include <vector>

#include <boost/filesystem.hpp>


namespace hyper {
	namespace compiler {
		class universe;
		struct recipe_decl_list;
		struct task_decl_list;

		class parser {
			private:
				universe &u;
				std::vector < boost::filesystem::path > parsed_files;

			public:
				parser(universe & u_) : u(u_) {};
				bool parse_ability_file(const std::string&);
				bool parse_expression(const std::string&);
				bool parse_task(const std::string&);
				bool parse_task(const std::string&, task_decl_list&);
				bool parse_task_file(const std::string&);
				bool parse_task_file(const std::string&, task_decl_list&);
				bool parse_recipe(const std::string&);
				bool parse_recipe(const std::string&, recipe_decl_list&);
				bool parse_recipe_file(const std::string&);
				bool parse_recipe_file(const std::string&, recipe_decl_list&);
		};
	}
}

#endif /* _PARSER_HH_ */
