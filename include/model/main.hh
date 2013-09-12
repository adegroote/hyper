#ifndef HYPER_MODEL_MAIN_HH_
#define HYPER_MODEL_MAIN_HH_

#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include <boost/system/system_error.hpp>

#include <model/discover_root.hh>

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
			} 
            catch (const hyper::model::root_not_found_error&) 
            {
                std::cerr << "Can't find hyperruntime address, "
                              "please set HYPER_ROOT_ADDR or "
                              "configure ${HOME}/.hyper/config" << std::endl;
                return -1;
            }
            catch (const boost::system::system_error& e) {
                if (e.code() == boost::system::errc::connection_refused)
                {
                    std::cerr << "Failed to connect to hyperruntime!\n"
                                 "Check that it is started, and that "
                                 "your configuration (HYPER_ROOT_ADDR,"
                                 " or ${HOME}/.hyper/config) matches your"
                                 " real hyperruntime setting" << std::endl;
                } else {
                    std::cerr << "Fatal error: " << e.what() << " Exiting!" << std::endl;
                }

                return -1;
			}

			return 0;
		}
	}
}

#endif /* HYPER_MODEL_MAIN_HH_ */
