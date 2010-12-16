#include <model/ability.hh>
#include <model/task.hh>
#include <model/recipe.hh>

namespace {
	using namespace hyper::model;
	struct callback {
		ability & a;
		bool res;

		callback(ability& a, bool res) : a(a), res(res) {}	

		void operator()(task_execution_callback cb) { 
			a.io_s.post(boost::bind(cb, res));
		}
	};
}

namespace hyper {
	namespace model {
		
		void task::handle_final_postcondition_handle(conditionV failed)
		{
			end_execute(failed.empty());
		}

		void task::handle_execute(bool res)
		{
			if (!res)
				end_execute(res); // XXX Add the error scope

			// check the post-conditions to be sure that everything is ok

			return async_evaluate_postconditions(boost::bind(
						&task::handle_final_postcondition_handle,
						this, _1));
		}

		void task::async_evaluate_recipe_preconditions(conditionV failed, size_t i)
		{
			recipe_states[i].failed = failed;
			recipe_states[i].index = i;
			recipe_states[i].nb_preconds = 0; // XXX

			if (i < recipes.size()) {
				return recipes[i+1]->async_evaluate_preconditions(
						boost::bind(&task::async_evaluate_recipe_preconditions,
									this, _1, i+1));
			} else {
				// time to select a recipe and execute it :)
				std::sort(recipe_states.begin(), recipe_states.end());
				if (! recipe_states[0].failed.empty())
					end_execute(false);

				return recipes[recipe_states[0].index]->execute(
						boost::bind(&task::handle_execute, this, _1));
			}
		}

		void task::handle_precondition_handle(conditionV failed)
		{
			/* If some pre-conditions are not ok, something is wrong, just
			 * returns false */

			if (!failed.empty())
				end_execute(false);

			if (recipes.empty())
				end_execute(false);

			/* compute precondition for all recipe */
			recipes[0]->async_evaluate_preconditions(
					boost::bind(&task::async_evaluate_recipe_preconditions,
								this, _1, 0));
		}

		void task::handle_initial_postcondition_handle(conditionV failed)
		{
			/* If all post-conditions are ok, no need to execute the task
			 * anymore */
			if (failed.empty())
				end_execute(true);

			return async_evaluate_preconditions(boost::bind(
						&task::handle_precondition_handle, 
						this, _1));
		}

		void task::execute(task_execution_callback cb)
		{
			pending_cb.push_back(cb);
			if (is_running)
				return;

			is_running = true;

			return async_evaluate_postconditions(boost::bind(
						&task::handle_initial_postcondition_handle, 
						this, _1));
		}

		void task::end_execute(bool res)
		{
			is_running = false;

			std::for_each(pending_cb.begin(), pending_cb.end(),
					callback(a, res));

			pending_cb.clear();
		}
	}
}
