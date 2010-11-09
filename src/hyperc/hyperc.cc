#include <hyperConfig.hh>

#include <iostream>
#include <fstream>
#include <set>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <compiler/depends.hh>
#include <compiler/parser.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>
#include <compiler/task.hh>
#include <compiler/task_parser.hh>
#include <compiler/recipe.hh>
#include <compiler/recipe_parser.hh>

using namespace hyper::compiler;
using namespace boost::filesystem;
namespace po = boost::program_options;

void
usage(const po::options_description& desc)
{
	std::cout << "Usage: hyperc [options]\n";
	std::cout << desc;
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

void build_swig(std::ostream& oss, const std::string& name)
{
	std::string swig = 
		"%module @NAME@\n"
		"%include \"typemaps.i\"\n"
		"%{\n"
		"	#include <string>\n"
		"	#include \"@NAME@/export.hh\"\n"
		"	using namespace hyper::@NAME@;\n"
		"%}\n"
		"%include \"std_string.i\"\n"
		"%include \"@NAME@/types.hh\"\n"
		"%include \"@NAME@/export.hh\"\n"
		;

	oss << hyper::compiler::replace_by(swig, "@NAME@", name);

}

void build_base_cmake(std::ostream& oss, const std::string& name, bool has_func,
					  bool has_task, bool has_recipe,
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
		"install(FILES @NAME@/funcs.hh @NAME@/import.hh\n"
		"		 DESTINATION ${HYPER_ROOT}/include/hyper/@NAME@/)\n"
		"\n"
		;

	std::string base_src=
		"set(SRC main.cc)\n";

	std::string recipe_src=
		"file(GLOB recipes @NAME@/recipes/*cc)\n"
		"set(SRC ${recipes} ${SRC})\n"
		;

	std::string task_src=
		"file(GLOB tasks @NAME@/tasks/*cc)\n"
		"set(SRC ${tasks} ${SRC})\n"
		;

	std::string build_ability = 
		"add_executable(@NAME@ ${SRC})\n"
		"install(TARGETS @NAME@\n"
		"		 DESTINATION ${HYPER_ROOT}/bin)\n"
		"install(FILES @NAME@/types.hh\n"
		"		 DESTINATION ${HYPER_ROOT}/include/hyper/@NAME@)\n"
		"install(FILES @NAME@.ability\n"
		"		 DESTINATION ${HYPER_ROOT}/share/hyper)\n"
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
		"target_link_libraries(@NAME@ ${HYPER_ROOT}/lib/libhyper_model.so)\n"
		"target_link_libraries(@NAME@ ${HYPER_ROOT}/lib/libhyper_logic.so)\n"
		;

	std::string swig_export =
		"find_package(SWIG)\n"
		"include(${SWIG_USE_FILE})\n"
		"find_package(Ruby)\n"
		"include_directories(${RUBY_INCLUDE_PATH})\n"
		"add_custom_command(\n"
		"	OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/@NAME@_wrap.cpp\n"
		"	COMMAND ${SWIG_EXECUTABLE}\n" 
		"ARGS -c++ -Wall -v -ruby -prefix \"hyper::\"  -I${Boost_INCLUDE_DIRS} -I${RUBY_INCLUDE_DIR}  -o ${CMAKE_CURRENT_BINARY_DIR}/@NAME@_wrap.cpp ${CMAKE_CURRENT_SOURCE_DIR}/@NAME@.i\n"
		"MAIN_DEPENDENCY @NAME@.i\n"
		"WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}\n"
		")\n"
	"add_library(@NAME@_ruby_wrap SHARED ${CMAKE_CURRENT_BINARY_DIR}/@NAME@_wrap.cpp @NAME@/export.cc)\n"
		"target_link_libraries(@NAME@_ruby_wrap stdc++ ${RUBY_LIBRARY})\n"
		"target_link_libraries(@NAME@_ruby_wrap ${HYPER_ROOT}/lib/libhyper_network.so)\n"
		"set_target_properties(@NAME@_ruby_wrap \n"
		"						PROPERTIES OUTPUT_NAME @NAME@\n"
		"						PREFIX \"\")\n"
		"EXECUTE_PROCESS(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e \"print Config::CONFIG['sitearch']\"\n"
		"				OUTPUT_VARIABLE RUBY_SITEARCH)\n"
		"string(REGEX MATCH \"[0-9]+\\\\.[0-9]+\" RUBY_VERSION \"${RUBY_VERSION}\")\n"
		"install(TARGETS @NAME@_ruby_wrap\n"
		"			DESTINATION ${HYPER_ROOT}/lib/ruby/${RUBY_VERSION}/${RUBY_SITEARCH}/hyper)\n"
		;


	oss << hyper::compiler::replace_by(base_cmake1, "@NAME@", name);
	if (has_func)
		oss << hyper::compiler::replace_by(build_function, "@NAME@", name);

	oss << base_src;
	if (has_task)
		oss << hyper::compiler::replace_by(task_src, "@NAME@", name);
	if (has_recipe)
		oss << hyper::compiler::replace_by(recipe_src, "@NAME@", name);
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

	oss << hyper::compiler::replace_by(swig_export, "@NAME@", name);
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
	path src = base_src / current_path;
	path dst = base_dst / current_path;

	create_directory(dst);

	directory_iterator end_itr; 
	for (directory_iterator itr( src ); itr != end_itr; ++itr ) {
		if (is_regular_file(itr->path())) {
			path src_file = src / itr->path().filename();
			path dst_file = dst / itr->path().filename();
			if (!are_file_equals(src_file, dst_file)) {
				std::cout << "copying " << src_file << " to " << dst_file << std::endl;
				// XXX there is to have a bug when overwriting files, so remove
				// the dst_file before copying the new one
				if (exists(dst_file))
					remove(dst_file);
				copy_file(src_file, dst_file, copy_option::overwrite_if_exists);
			}

		} else if (is_directory(itr->path())) {
			path current = current_path / itr->path().filename();
			copy_if_different(base_src, base_dst, current);
		}
	}
}

bool is_directory_empty(const path& directory)
{
	directory_iterator end_itr;
	directory_iterator itr(directory);
	return (itr != end_itr);
}

struct generate_recipe 
{
	const universe& u;
	const ability& ab;
	const typeList& tList;
	task t;

	bool success;
	std::string directoryName;

	generate_recipe(const universe& u_,
					const std::string& abilityName, 
					const std::string& directoryName_) :
		u(u_), ab(u.get_ability(abilityName)), tList(u.types()), 
		t(task_decl(), ab, tList),
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

struct generate_task
{
	const universe& u;
	ability& ab;
	const typeList& tList;

	bool success;
	std::string directoryName;

	generate_task(universe& u_,
				  const std::string& abilityName, 
				  const std::string& directoryName_) :
		u(u_), ab(u_.get_ability(abilityName)), tList(u.types()),
		success(true), directoryName(directoryName_)
	{}

	void operator() (const task_decl& decl)
	{
		task t(decl, ab, tList);
		bool valid = t.validate(u);
		if (!valid) {
			std::cerr << t.get_name() << " seems not valid " << std::endl;
		}
		valid &= ab.add_task(t);
		success = success && valid;
		if (success) {
			std::string fileName = directoryName + "/" + t.get_name() + ".cc";
			std::ofstream oss(fileName.c_str());
			t.dump(oss, u);
		}
	}
};

int main(int argc, char** argv)
{
	try {
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("verbose,v", po::value<int>()->implicit_value(1),
		 "enable verbosity (optionally specify level)")
		("include-path,I", po::value< std::vector<std::string> >(), 
		 "include path")
		("input-file", po::value< std::string >(), "input file")
		;

	po::positional_options_description p;
	p.add("input-file", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		usage(desc);
		return 0;
	}

	if (!vm.count("input-file"))
	{
		usage(desc);
		return -1;
	}

	std::vector<std::string> include_dirs = vm["include-path"].as < std::vector<std::string> >();
	std::vector<path> include_dir_path;
	std::copy(include_dirs.begin(), include_dirs.end(), std::back_inserter(include_dir_path));

	std::string abilityName = vm["input-file"].as < std::string > ();
	std::string baseName = ".hyper/src/";
	std::string directoryName = baseName + abilityName;
	std::string directoryTaskName = directoryName + "/tasks";
	std::string directoryRecipeName = directoryName + "/recipes";
	std::string taskDirectory = abilityName;

	universe u;
	parser P(u, include_dir_path);

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
				   task_decl_list task_decls;
				   res = P.parse_task_file(itr->path().string(), task_decls);
				   if (res == false) {
					   std::cerr << "Fail to parse " << itr->path().string() << std::endl;
					   return -1;
				   }
				   generate_task gen(u, abilityName, directoryTaskName);
				   std::for_each(task_decls.list.begin(), 
								 task_decls.list.end(),
								 gen);
				   if (gen.success == false)
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

	const ability& current_a = u.get_ability(abilityName);

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
		std::string fileName = baseName + "main.cc";
		std::ofstream oss(fileName.c_str());
		build_main(oss, abilityName);
	}

	{
		std::string fileName = baseName + "CMakeLists.txt";
		std::ofstream oss(fileName.c_str());
		depends d = u.get_function_depends(abilityName);
		build_base_cmake(oss, abilityName, define_func, 
						 is_directory_empty(directoryTaskName),
						 is_directory_empty(directoryRecipeName),
						 d.fun_depends);
	}

	{
		std::string fileName = directoryName + "/export.hh";
		std::ofstream oss(fileName.c_str());
		current_a.agent_export_declaration(oss, u.types());
	}
	{
		std::string fileName = directoryName + "/export.cc";
		std::ofstream oss(fileName.c_str());
		current_a.agent_export_implementation(oss, u.types());
	}
	{ 
		std::string fileName = baseName + abilityName + ".i";
		std::ofstream oss(fileName.c_str());
		build_swig(oss, abilityName);
	}

	/* Now for all files in .hyper/src, copy different one into real src */
	copy_if_different(baseName, "src", "");

	/* copy ability file in src to install it */
	std::string abilityFileName = abilityName + ".ability";
	std::string dst_abilityFileName = "src/" + abilityFileName;

	if (exists(dst_abilityFileName))
		remove(dst_abilityFileName);
	copy_file(abilityFileName, dst_abilityFileName);
	} catch (std::exception &e) { 
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}
