#ifndef HYPER_MODEL_LOGIC_LAYER_FWD_HH_
#define HYPER_MODEL_LOGIC_LAYER_FWD_HH_

#include <boost/shared_ptr.hpp>

namespace hyper {
	namespace model {
		struct logic_context;
		typedef boost::shared_ptr<logic_context> logic_ctx_ptr;

		struct logic_layer;
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_FWD_HH_ */
