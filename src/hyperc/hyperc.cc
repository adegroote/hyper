#include <hyperConfig.hh>

#include <iostream>

#include <compiler/parser.hh>
#include <compiler/universe.hh>

using namespace hyper::compiler;

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

	return 0;
}
