#include <hyperConfig.hh>

#include <iostream>
#include <fstream>

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

void build_base_cmake(std::ostream& oss, const std::string& name)
{
	std::string cmake = 
		"cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)\n"
		"\n"
		"project(HYPER_ABILITY_@NAME@ CXX)\n"
		"enable_language(C)\n"
		"include(CheckIncludeFile)\n"
		"\n"
		"find_package(Boost 1.42 REQUIRED COMPONENTS system thread serialization)\n"
		"set(BOOST_FOUND ${Boost_FOUND})\n"
		"include_directories(${Boost_INCLUDE_DIRS})\n"
		"message(STATUS \"boost libraries \"${Boost_LIBRARIES})\n"
		"\n"
		"include_directories(${CMAKE_SOURCE_DIR})\n"
		"add_executable(@NAME@ main.cc)\n"
		"target_link_libraries(@NAME@ ${Boost_LIBRARIES})\n"

		"include_directories(${HYPER_ROOT}/include/hyper)\n"
		"target_link_libraries(@NAME@ ${HYPER_ROOT}/lib/libhyper_network.so)\n"
		;

	oss << hyper::compiler::replace_by(cmake, "@NAME@", name);
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

	{
		std::string fileName = directoryName + "/funcs.hh";
		std::ofstream oss(fileName.c_str());
		if ( u.dump_ability_functions_proto(oss, abilityName) == 0)
			remove(fileName);
		else {
			std::string fileNameImpl = directoryName + "/funcs.cc";
			if (exists(fileNameImpl)) {
				// don't touch to the current funcs.cc, write a template one
				fileNameImpl = directoryName + "/funcs.template.cc";
			}
			std::ofstream oss_impl(fileNameImpl.c_str());
			u.dump_ability_functions_impl(oss_impl, abilityName);
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
		build_base_cmake(oss, abilityName);
	}



	return 0;
}
