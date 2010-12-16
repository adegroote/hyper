#ifndef HYPER_MODEL_LOGIC_LAYER_FWD_HH_
#define HYPER_MODEL_LOGIC_LAYER_FWD_HH_

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>

namespace hyper {
	namespace model {
		struct logic_context;
		typedef boost::shared_ptr<logic_context> logic_ctx_ptr;

		struct logic_layer;

		typedef boost::function<void (const boost::system::error_code&)> 
				logic_layer_cb;
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_FWD_HH_ */
