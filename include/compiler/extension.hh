#ifndef HYPER_COMPILER_EXTENSION_HH_
#define HYPER_COMPILER_EXTENSION_HH_

#include <sstream>

namespace hyper {
	namespace compiler {
		struct functionDef;
		struct typeList;

		struct extension {

			virtual void function_proto(std::ostream& oss, const functionDef& f,
												   const typeList& tList) const = 0;
			virtual void function_impl(std::ostream& oss, const functionDef& f,
												   const typeList& tList) const = 0;

			virtual ~extension() {}
		};
	}
}

#endif /* HYPER_COMPILER_EXTENSION_HH_ */
