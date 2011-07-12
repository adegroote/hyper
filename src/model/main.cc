#include <model/main.hh>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace {
	
		void usage(const po::options_description& desc, const std::string& name)
		{
			std::cout << "Usage: " << name << "[options]\n";
			std::cout << desc;
		}
}

namespace hyper {
	namespace model {
		void parse_options(int argc, char** argv, const std::string& name, bool& background,
																		   int& debug_lvl)
		{
			po::options_description desc("Allowed options");
			desc.add_options()
			("background,b", "run in background")
			("help,h", "produce help message")
			("debug_level,d", po::value<int>(&debug_lvl)->implicit_value(0),
			 "enable verbosity (optionally specify level)")
			;

			po::variables_map vm;
			po::store(po::command_line_parser(argc, argv).
					options(desc).run(), vm);
			po::notify(vm);

			if (vm.count("help")) {
				usage(desc, name);
				exit(0);
			}

			background = (vm.count("background") != 0);
		}
	}
}
