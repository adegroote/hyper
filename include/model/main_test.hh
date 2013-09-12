#ifndef HYPER_MODEL_MAIN_TEST_HH_
#define HYPER_MODEL_MAIN_TEST_HH_

#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>

#include <boost/system/system_error.hpp>

#include <compiler/import_exception.hh>
#include <model/discover_root.hh>

namespace hyper {
	namespace model {
		template <typename Agent>
		int main_test(int argc, char** argv, const std::string& name)
		{
			try {
                if (! Agent::correct_usage(argc, argv))
                    return Agent::usage(name);
				Agent agent;
				return agent.main(argc, argv);
			} 
            catch (const hyper::model::root_not_found_error&) 
            {
                std::cerr << "Can't find hyperruntime address, "
                              "please set HYPER_ROOT_ADDR or "
                              "configure ${HOME}/.hyper/config" << std::endl;
                return -1;
            }
            catch (const hyper::compiler::import_exception &e)
            {
                std::cerr << e.what() << std::endl;
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
		}
	}
}

#endif /* HYPER_MODEL_MAIN_TEST_HH_ */
