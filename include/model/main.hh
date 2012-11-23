#ifndef HYPER_MODEL_MAIN_HH_
#define HYPER_MODEL_MAIN_HH_

#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include <boost/system/system_error.hpp>

namespace hyper {
	namespace model {

		void parse_options(int argc, char** argv, const std::string& name,
						   bool& background,
						   int& debug_lvl);

		template <typename Agent>
		int main(int argc, char** argv, const std::string& name)
		{
			try {
				int level;
				bool bg;
				parse_options(argc, argv, name, bg, level);
				// XXX NON PORTABLE IMPLEMENTATION
				if (bg) { 
					if (daemon(1, 1) < 0) 
						throw boost::system::system_error(
								boost::system::error_code(errno, boost::system::system_category()));
								  
				}
				Agent agent(level);
				agent.run();
			} catch (boost::system::system_error& e) {
				std::cerr << "Catched exception : " << e.what() << std::endl;
				exit(-1);
			}
		}
	}
}

#endif /* HYPER_MODEL_MAIN_HH_ */
