#ifndef HYPER_MODEL_RECIPE_HH_
#define HYPER_MODEL_RECIPE_HH_

#include <vector>

#include <boost/thread.hpp>

#include <model/task_fwd.hh>
#include <model/recipe_fwd.hh>

namespace hyper {
	namespace model {
		struct ability;

		class recipe {
			private:
				std::string name;
				ability &a;
				bool is_running;
				std::vector<recipe_execution_callback> pending_cb;
				boost::thread* thr;

				void real_execute();
				void handle_execute(bool res);
				void handle_evaluate_preconditions(conditionV error);
				void end_execute(bool res);

			protected:
				virtual bool do_execute() = 0;
			public:
				recipe(const std::string& name, ability& a);
				void execute(recipe_execution_callback cb);
				virtual void async_evaluate_preconditions(condition_execution_callback cb) = 0;
				virtual ~recipe();
		};
	}
}

#endif /* HYPER_MODEL_RECIPE_HH_ */
