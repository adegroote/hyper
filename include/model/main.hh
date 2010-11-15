#ifndef HYPER_MODEL_MAIN_HH_
#define HYPER_MODEL_MAIN_HH_

#include <iostream>
#include <string>

#include <boost/system/system_error.hpp>

namespace hyper {
	namespace model {

		void parse_options(int argc, char** argv, const std::string& name,
						   int& debug_lvl);

		template <typename Agent>
		int main(int argc, char** argv, const std::string& name)
		{
			try {
				int level;
				parse_options(argc, argv, name, level);
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
