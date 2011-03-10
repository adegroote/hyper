#include <cstdlib>      
#include <iostream>
#include <fstream>

#include <model/discover_root.hh>

namespace hyper {
	namespace model {
		discover_root::discover_root() 
		{
			// first get to read HYPER_ROOT_ADDR environment variable
			const char* addr_ = std::getenv("HYPER_ROOT_ADDR");
			if (addr_ != NULL) {
				std::string addr(addr_);
				size_t found = addr.find(":");
				if (found != std::string::npos && found != 0) {
					root_addr_ = addr.substr(0, found);
					root_port_ = addr.substr(found+1);
					return;
				}
			}

			// try to read ~/.hyper/config, first line must be host port 
			std::string home = std::getenv("HOME");
			std::string path = home + "/.hyper/config";
			std::ifstream ifs(path.c_str());
			if (ifs.is_open()) {
				ifs >> root_addr_ >> root_port_;
				return;
			} else {
				std::cerr << "Can't open " << path; 
				std::cerr << " to read configuration " << std::endl;
			}

			// think about adding multicast search / bonjour mechanism


			throw root_not_found_error();
		}
	}
}
