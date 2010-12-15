#ifndef HYPER_MODEL_RECIPE_FWD_HH_
#define HYPER_MODEL_RECIPE_FWD_HH_

#include <boost/shared_ptr.hpp>

namespace hyper {
	namespace model {
		struct recipe;
		typedef boost::shared_ptr<recipe> recipe_ptr;
		typedef boost::function<void (bool)> recipe_execution_callback; 
	}
}

#endif /* HYPER_MODEL_RECIPE_FWD_HH_ */
