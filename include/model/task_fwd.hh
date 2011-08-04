#ifndef HYPER_MODEL_TASK_FWD_HH_
#define HYPER_MODEL_TASK_FWD_HH_

#include <boost/function/function1.hpp>
#include <boost/function/function2.hpp>
#include <boost/shared_ptr.hpp>

#include <model/eval_conditions_fwd.hh>

namespace hyper {
	namespace model {
		typedef boost::function<void (const boost::system::error_code&e, conditionV)> condition_execution_callback;
		typedef boost::function<void (bool)> task_execution_callback;
		struct task;
		typedef boost::shared_ptr<task> task_ptr;
	}
}

#endif /* HYPER_MODEL_TASK_FWD_HH_ */
