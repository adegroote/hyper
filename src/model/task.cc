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
				return end_execute(res); // XXX Add the error scope

			// check the post-conditions to be sure that everything is ok
			return async_evaluate_postconditions(boost::bind(
						&task::handle_final_postcondition_handle,
						this, _1));
		}

		void task::async_evaluate_recipe_preconditions(conditionV failed, size_t i)
		{
			recipe_states[i].failed = failed;
			recipe_states[i].index = i;
			recipe_states[i].nb_preconds = 0; // XXX<

			if (i != (recipes.size() - 1)) {
				boost::optional<size_t> electable;
				for (size_t j = i+1; j < recipes.size() && !electable; ++j)
					if (recipe_states[j].missing_agents.empty())
						electable = j;

				if (electable)
					return recipes[i+1]->async_evaluate_preconditions(
							boost::bind(&task::async_evaluate_recipe_preconditions,
								this, _1, i+1));
			} 

			// time to select a recipe and execute it :)
			std::sort(recipe_states.begin(), recipe_states.end());
			if (! recipe_states[0].failed.empty() &&
				! recipe_states[0].missing_agents.empty() )
				return end_execute(false);

			return recipes[recipe_states[0].index]->execute(
					boost::bind(&task::handle_execute, this, _1));
		}

		void task::handle_precondition_handle(conditionV failed)
		{
			/* If some pre-conditions are not ok, something is wrong, just
			 * returns false */

			if (!failed.empty())
				return end_execute(false);

			if (recipes.empty())
				return end_execute(false);

			boost::optional<size_t> first_electable;
			/* Evaluate the computable recipes, depending on the availables agent */
			for (size_t i = 0; i < recipes.size(); ++i) {
				recipe_states[i].index = i;
				recipe_states[i].missing_agents.clear();
				std::set_difference(recipes[i]->begin(), recipes[i]->end(), 
									a.alive_agents.begin(), a.alive_agents.end(),
									std::inserter(recipe_states[i].missing_agents, 
												  recipe_states[i].missing_agents.end()));

				if (recipe_states[i].missing_agents.empty() && !first_electable) 
					first_electable = i;
				
			}

			if (!first_electable) // no computable recipe
				return end_execute(false);

			/* compute precondition for all recipe */
			recipes[*first_electable]->async_evaluate_preconditions(
					boost::bind(&task::async_evaluate_recipe_preconditions,
								this, _1, 0));
		}

		void task::handle_initial_postcondition_handle(conditionV failed)
		{
			/* If all post-conditions are ok, no need to execute the task
			 * anymore */
			if (failed.empty())
				return end_execute(true);

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


			/* Handle correctly the special case where we don't have
			 * postcondition.  It means that they can't be called using the
			 * logic solver, but that the call has been forced (by an update
			 * stuff for example). In this case, don't report ok now, try to
			 * really execute it */
			if (has_postconditions())
				return async_evaluate_postconditions(boost::bind(
							&task::handle_initial_postcondition_handle, 
							this, _1));
			else
				return async_evaluate_preconditions(boost::bind(
							&task::handle_precondition_handle, 
							this, _1));

		}

		void task::end_execute(bool res)
		{
			is_running = false;

			std::for_each(pending_cb.begin(), pending_cb.end(),
					callback(a, res));

			pending_cb.clear();
		}

		void task::add_recipe(recipe_ptr ptr)
		{
			recipes.push_back(ptr);
			recipe_states.resize(recipes.size());
		}
	}
}
