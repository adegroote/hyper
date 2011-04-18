
#ifndef _COMPILER_UTIL_HH_
#define _COMPILER_UTIL_HH_

#include <algorithm>
#include <string>

namespace hyper {
	namespace compiler {
		std::string read_from_file(const std::string&);

		/*
		 * Replace all occurrence of @pattern by @replace in @str
		 */
		std::string replace_by(const std::string& , const std::string& pattern,	
													const std::string& replace);
		
		std::string upper(const std::string& str);
	}
}

#endif /* _COMPILER_UTIL_HH_ */
