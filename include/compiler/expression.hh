
#ifndef _HYPER_EXPRESSION_HH_
#define _HYPER_EXPRESSION_HH_

#include <vector>

#include <boost/variant.hpp>

namespace hyper {
	namespace compiler {
		template <typename T>
		struct Constant
		{
			T value;
		};

		struct function_call;

		typedef boost::variant <
			Constant<int>,					// constant of type int
			Constant<double>,				// constant of type double
			Constant<std::string>,			// constant of type string
			std::string,					// variable identifier
			boost::recursive_wrapper<function_call> // function call
		> node;

		struct function_call {
			std::string fName;
			std::vector< node > args;
		};

		std::ostream& operator << (std::ostream& os, const node& n);
	};
};



#endif /* _HYPER_EXPRESSION_HH_ */
