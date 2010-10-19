#include <hyperConfig.hh>

#include <iostream>
#include <fstream>
#include <set>

#include <boost/filesystem.hpp>

#include <compiler/parser.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>
#include <compiler/recipe.hh>
#include <compiler/recipe_parser.hh>

using namespace hyper::compiler;
using namespace boost::filesystem;

void
usage() 
{
	std::cerr << "Usage: hyperc <ability>" << std::endl;
}

void build_main(std::ostream& oss, const std::string& name)
{
	std::string main = 
		"#include <iostream>\n"
		"#include <@NAME@/ability.hh>\n"
		"\n"
		"int main()\n"
		"{\n"
		"	try { \n"
		"		hyper::@NAME@::ability @NAME@;\n"
		"		@NAME@.run();\n"
		"	} catch (boost::system::system_error& e) {\n"
		"		std::cerr << \"Catched exception from ability @NAME@ :\";\n"
		"		std::cerr << e.what() << std::endl;\n"
		"	}\n"
		"}\n"
	;

	oss << hyper::compiler::replace_by(main, "@NAME@", name);
}

void build_base_cmake(std::ostream& oss, const std::string& name, bool has_func,
					  const std::set<std::string>& depends)
{
	std::string base_cmake1 = 
		"cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)\n"
		"\n"
		"project(HYPER_ABILITY_@NAME@ CXX)\n"
		"enable_language(C)\n"
		"include(CheckIncludeFile)\n"
		"\n"
		"find_package(Boost 1.42 REQUIRED COMPONENTS system thread serialization filesystem)\n"
		"set(BOOST_FOUND ${Boost_FOUND})\n"
		"include_directories(${Boost_INCLUDE_DIRS})\n"
		"message(STATUS \"boost libraries \"${Boost_LIBRARIES})\n"
		"\n"
		"include_directories(${CMAKE_SOURCE_DIR})\n"
		"\n"
		;
	std::string build_function = 
		"add_library(hyper_@NAME@ SHARED @NAME@/funcs.cc @NAME@/import.cc)\n"
		"install(TARGETS hyper_@NAME@\n"
		"		  DESTINATION ${HYPER_ROOT}/lib/hyper/\n"
		")\n"
		"\n"
		;

	std::string build_ability = 
		"add_executable(@NAME@ main.cc)\n"
		;
	std::string link_function =
		"target_link_libraries(@NAME@ hyper_@NAME@)\n"
		;

	std::string link_depends =
		"target_link_libraries(@NAME@ ${HYPER_ROOT}/lib/hyper/libhyper_"
		;

	std::string additionnal_link = 
		"target_link_libraries(@NAME@ ${Boost_LIBRARIES})\n"

		"include_directories(${HYPER_ROOT}/include/hyper)\n"
		"target_link_libraries(@NAME@ ${HYPER_ROOT}/lib/libhyper_network.so)\n"
		"target_link_libraries(@NAME@ ${HYPER_ROOT}/lib/libhyper_compiler.so)\n"
		"target_link_libraries(@NAME@ ${HYPER_ROOT}/lib/libhyper_logic.so)\n"
		;

	oss << hyper::compiler::replace_by(base_cmake1, "@NAME@", name);
	if (has_func)
		oss << hyper::compiler::replace_by(build_function, "@NAME@", name);

	oss << hyper::compiler::replace_by(build_ability, "@NAME@", name);
	if (has_func)
		oss << hyper::compiler::replace_by(link_function, "@NAME@", name);

	std::set<std::string>::const_iterator it;
	for (it = depends.begin(); it != depends.end(); ++it) {
		if (*it != name) {
			oss << hyper::compiler::replace_by(link_depends, "@NAME@", name);
			oss << *it << ".so)\n";
		}
	}

	oss << hyper::compiler::replace_by(additionnal_link, "@NAME@", name);
}

/*
 * Assume src always exists
 */
bool are_file_equals(const path& src, const path& dst)
{
	if (!exists(dst)) 
		return false;

	std::string src_content = read_from_file(src.string());
	std::string dst_content = read_from_file(dst.string());

	return (src_content == dst_content);
}

void copy_if_different(const path& base_src, const path& base_dst,
					   const path& current_path)
{
	path src(base_src);
	src /= current_path;

	path dst(base_dst);
	dst /= current_path;

	create_directory(dst);

	directory_iterator end_itr; 
	for (directory_iterator itr( src ); itr != end_itr; ++itr ) {
		if (is_regular_file(itr->path())) {
			path src_file(src);
			src_file /= itr->path().filename();
			path dst_file(dst);
			dst_file /= itr->path().filename();
			if (!are_file_equals(src_file, dst_file)) {
				std::cout << "copying " << src_file << " to " << dst_file << std::endl;
				copy_file(src_file, dst_file, copy_option::overwrite_if_exists);
			}

		} else if (is_directory(itr->path())) {
			path current(current_path);
			current /= itr->path().filename();
			copy_if_different(base_src, base_dst, current);
		}
	}
}

struct generate_recipe 
{
	const universe& u;
	const ability& ab;
	task t;
	const typeList& tList;

	bool success;
	std::string directoryName;

	generate_recipe(const universe& u_,
					const std::string& abilityName, 
					const std::string& directoryName_) :
		u(u_), ab(u.get_ability(abilityName)), tList(u.types()), 
		success(true), directoryName(directoryName_) 
	{}

	void operator() (const recipe_decl& decl)
	{
		recipe r(decl, ab, t, tList);
		bool valid = r.validate(u);
		if (!valid) {
			std::cerr << r.get_name() << " seems not valid " << std::endl;
		}
		success = success && valid;
		if (success) {
			std::string fileName = directoryName + "/" + r.get_name() + ".cc";
			std::ofstream oss(fileName.c_str());
			r.dump(oss, u);
		}
	}
};

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		usage();
		return -1;
	}

	std::string abilityName = argv[1];
	std::string directoryName = ".hyper/src/" + abilityName;
	std::string directoryTaskName = directoryName + "/tasks";
	std::string directoryRecipeName = directoryName + "/recipes";
	std::string taskDirectory = abilityName;
	universe u;
	parser P(u);

	create_directory(".hyper");
	create_directory(".hyper/src");
	create_directory(directoryName);	
	create_directory(directoryTaskName);
	create_directory(directoryRecipeName);

	bool res = P.parse_ability_file(abilityName + ".ability");
	if (res == false)
		return -1;

	if (exists(taskDirectory)) {
		  directory_iterator end_itr; // default construction yields past-the-end
		   for (directory_iterator itr( taskDirectory ); itr != end_itr; ++itr ) {
			   if (is_regular_file(itr->path())) {
				   res = P.parse_task_file(itr->path().string());
				   if (res == false)
					   return -1;
			   } else if (is_directory(itr->path())) {
					directory_iterator end_itr2; 
					for (directory_iterator itr2(*itr); itr2 != end_itr2; ++itr2 ) {
						if (is_regular_file(itr2->path())) {
							recipe_decl_list rec_decls;
							res = P.parse_recipe_file(itr2->path().string(), rec_decls);
							if (res == false) {
								std::cerr << "Fail to parse " << itr2->path().string() << std::endl;
								return -1;
							}

							generate_recipe gen(u, abilityName, directoryRecipeName);
							std::for_each(rec_decls.recipes.begin(),
										  rec_decls.recipes.end(),
										  gen);
							if (gen.success == false)
								return -1;
						}
					}
			   }
		   }
	}

	{
		std::string fileName = directoryName + "/types.hh";
		std::ofstream oss(fileName.c_str());
		if (u.dump_ability_types(oss, abilityName) == 0)
			remove(fileName);;
	}

	bool define_func = false;
	{
		std::string fileName = directoryName + "/funcs.hh";
		std::ofstream oss(fileName.c_str());
		if ( u.dump_ability_functions_proto(oss, abilityName) == 0) {
			remove(fileName);
			define_func = false;
		} else {
			std::string fileNameImpl = directoryName + "/funcs.cc";
			std::ofstream oss_impl(fileNameImpl.c_str());
			u.dump_ability_functions_impl(oss_impl, abilityName);
			define_func = true;

			{
				std::string fileName = directoryName + "/import.hh";
				std::ofstream oss(fileName.c_str());
				u.dump_ability_import_module_def(oss, abilityName);
			}

			{ 
				std::string fileName = directoryName + "/import.cc";
				std::ofstream oss(fileName.c_str());
				u.dump_ability_import_module_impl(oss, abilityName);
			}
		}
	}

	{
		std::string fileName = directoryName + "/ability.hh";
		std::ofstream oss(fileName.c_str());
		u.dump_ability(oss, abilityName);
	}

	{
		std::ofstream oss(".hyper/src/main.cc");
		build_main(oss, abilityName);
	}

	{
		std::ofstream oss(".hyper/src/CMakeLists.txt");
		std::set<std::string> depends = u.get_function_depends(abilityName);
		build_base_cmake(oss, abilityName, define_func, depends);
	}

	/* Now for all files in .hyper/src, copy different one into real src */
	copy_if_different(".hyper/src", "src", "");

	return 0;
}
