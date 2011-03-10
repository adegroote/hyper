#ifndef HYPER_DISCOVER_ROOT_HH_
#define HYPER_DISCOVER_ROOT_HH_

#include <stdexcept>

namespace hyper {
	namespace model {
		struct root_not_found_error : public std::runtime_error {
			root_not_found_error() : 
				std::runtime_error("Hyper::Root not found!")
			{}
		};

		class discover_root {
			public:
				/* Will throw a root_not_found_error if we can't find the hyper
				 * root */
				discover_root();
				const std::string& root_addr() const
				{
					return root_addr_;
				};
				const std::string& root_port() const
				{
					return root_port_;
				};

			private:
				std::string root_addr_, root_port_;
		};
	}
}

#endif /* HYPER_DISCOVER_ROOT_HH_ */
