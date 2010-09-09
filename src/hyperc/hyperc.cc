#include <hyperConfig.hh>

#include <iostream>
#include <fstream>
#include <set>

#include <boost/filesystem.hpp>

#include <compiler/parser.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>

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
		"add_library(hyper_@NAME@ SHARED @NAME@/funcs.cc)\n"
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

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		usage();
		return -1;
	}

	std::string abilityName = argv[1];
	std::string directoryName = "src/" + abilityName;
	universe u;
	parser P(u);

	create_directory("src");
	create_directory(directoryName);	

	bool res = P.parse_ability_file(abilityName + ".ability");
	if (res == false)
		return -1;

	res = P.parse_task_file(abilityName + ".task");
	if (res == false)
		return -1;

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
			if (exists(fileNameImpl)) {
				// don't touch to the current funcs.cc, write a template one
				fileNameImpl = directoryName + "/funcs.template.cc";
			}
			std::ofstream oss_impl(fileNameImpl.c_str());
			u.dump_ability_functions_impl(oss_impl, abilityName);
			define_func = true;
		}
	}

	{
		std::string fileName = directoryName + "/ability.hh";
		std::ofstream oss(fileName.c_str());
		u.dump_ability(oss, abilityName);
	}

	{
		std::ofstream oss("src/main.cc");
		build_main(oss, abilityName);
	}

	{
		std::ofstream oss("src/CMakeLists.txt");
		std::set<std::string> depends = u.get_function_depends(abilityName);
		build_base_cmake(oss, abilityName, define_func, depends);
	}



	return 0;
}
