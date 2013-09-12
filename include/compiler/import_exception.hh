#include <string>
#include <sstream>

namespace hyper {
	namespace compiler {
		struct import_exception {
			std::string filename;

			import_exception(const std::string& filename_) : filename(filename_) {}

			virtual const char* what() const = 0;
		};

		struct import_exception_not_found : public import_exception
		{
			import_exception_not_found(const std::string& filename) :
				import_exception(filename)
			{}

			virtual const char* what() const {
				std::ostringstream oss;
				oss << "Can't find the file " << filename << "\n";
				oss << "Please verify your include path "
                        "as specified by HYPER_INCLUDE_PATH "
                        "(and|or) explicit -I flag. See hyperc(1) " 
                        "for a complete description about the include "
                        "search algorithm." << std::endl;
				return oss.str().c_str();
			}
		};

		struct import_exception_parse_error : public import_exception
		{
			import_exception_parse_error(const std::string& filename) :
				import_exception(filename)
			{}

			virtual const char* what() const {
				std::ostringstream oss;
				oss << "Failed to parse " << filename << "!" << std::endl;
				return oss.str().c_str();
			}
		};
	}
}
