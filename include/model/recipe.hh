#ifndef HYPER_MODEL_RECIPE_HH_
#define HYPER_MODEL_RECIPE_HH_

#include <vector>
#include <set>

#include <boost/thread.hpp>

#include <model/abortable_function.hh>
#include <model/task_fwd.hh>
#include <model/recipe_fwd.hh>

namespace hyper {
	namespace model {
		struct ability;
		struct abortable_computation;

		class recipe {
			private:
				std::string name;
				ability &a;

				bool is_running;
				std::vector<recipe_execution_callback> pending_cb;

				void handle_evaluate_preconditions(conditionV error);
				void handle_execute(const boost::system::error_code& e);
				void end_execute(bool res);

			protected:
				abortable_computation* computation;
				virtual void do_execute(abortable_computation::cb_type cb) = 0;

				/* list of agents required to use this recipe */
				std::set<std::string> required_agents;

			public:
				typedef std::set<std::string>::const_iterator agent_const_iterator;

				recipe(const std::string& name, ability& a);
				void execute(recipe_execution_callback cb);
				virtual void async_evaluate_preconditions(condition_execution_callback cb) = 0;

				agent_const_iterator begin() const { return required_agents.begin(); }
				agent_const_iterator end() const { return required_agents.end(); }

				virtual ~recipe() {};
		};
	}
}

#endif /* HYPER_MODEL_RECIPE_HH_ */
