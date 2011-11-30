#ifndef HYPER_MODEL_LOGIC_LAYER_FWD_HH_
#define HYPER_MODEL_LOGIC_LAYER_FWD_HH_

#include <boost/function/function1.hpp>
#include <boost/shared_ptr.hpp>

#include <network/msg_constraint.hh>

namespace hyper {
	namespace model {
		struct logic_context;
		typedef boost::shared_ptr<logic_context> logic_ctx_ptr;

		struct logic_layer;

		typedef boost::function<void (network::request_constraint_answer::state_)>
				logic_layer_cb;
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_FWD_HH_ */
