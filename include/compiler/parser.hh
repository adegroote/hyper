
#ifndef _PARSER_HH_
#define _PARSER_HH_

#include <string>
#include <vector>

#include <boost/filesystem.hpp>


namespace hyper {
	namespace compiler {
		class universe;
		struct logic_expression_decl;
		struct recipe_decl_list;
		struct task_decl_list;

		class parser {
			public:
				typedef std::vector<boost::filesystem::path>::const_iterator path_iterator;

			private:
				universe &u;
				std::vector <boost::filesystem::path> parsed_files;
				std::vector<boost::filesystem::path> include_path;
				
				void get_include_path_from_env();

			public:
				parser(universe & u_) : u(u_) {
					get_include_path_from_env();
				}
				parser(universe & u_, const std::vector<boost::filesystem::path>& includes):
					u(u_), include_path(includes) {
					get_include_path_from_env();
				}
				bool parse_ability_file(const std::string&);
				bool parse_expression(const std::string&);
				bool parse_logic_expression(const std::string&);
				bool parse_logic_expression(const std::string&, logic_expression_decl&);
				bool parse_task(const std::string&);
				bool parse_task(const std::string&, task_decl_list&);
				bool parse_task_file(const std::string&);
				bool parse_task_file(const std::string&, task_decl_list&);
				bool parse_recipe(const std::string&);
				bool parse_recipe(const std::string&, recipe_decl_list&);
				bool parse_recipe_file(const std::string&);
				bool parse_recipe_file(const std::string&, recipe_decl_list&);

				path_iterator include_begin() const { return include_path.begin(); }
				path_iterator include_end() const { return include_path.end(); }


		};
	}
}

#endif /* _PARSER_HH_ */
