#include <hyperConfig.hh>

#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>

#include <compiler/parser.hh>
#include <compiler/universe.hh>

using namespace hyper::compiler;
using namespace boost::filesystem;

void
usage() 
{
	std::cerr << "Usage: hyperc <ability>" << std::endl;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		usage();
		return -1;
	}

	std::string abilityName = argv[1];
	universe u;
	parser P(u);

	bool res = P.parse_ability_file(abilityName + ".ability");
	if (res == false)
		return -1;

	res = P.parse_task_file(abilityName + ".task");
	if (res == false)
		return -1;

	create_directory(abilityName);	
	{
		std::string fileName = abilityName + "/types.hh";
		std::ofstream oss(fileName.c_str());
		if (u.dump_ability_types(oss, abilityName) == 0)
			remove(fileName);;
	}

	{
		std::string fileName = abilityName + "/funcs.hh";
		std::ofstream oss(fileName.c_str());
		if ( u.dump_ability_functions_proto(oss, abilityName) == 0)
			remove(fileName);
		else {
			std::string fileNameImpl = abilityName + "/funcs.cc";
			if (exists(fileNameImpl)) {
				// don't touch to the current funcs.cc, write a template one
				fileNameImpl = abilityName + "/funcs.template.cc";
			}
			std::ofstream oss_impl(fileNameImpl.c_str());
			u.dump_ability_functions_impl(oss_impl, abilityName);
		}
	}

	return 0;
}
