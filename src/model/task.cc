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

#define CHECK_INTERRUPT if (must_interrupt) return end_execute(false);

		void task::handle_final_postcondition_handle(conditionV failed)
		{
			CHECK_INTERRUPT

			if (!failed.empty()) {
				a.logger(DEBUG) << "[Task " << name << "] Still have some failed postcondition : \n";
				std::copy(failed.begin(), failed.end(),
						  std::ostream_iterator<std::string>(a.logger(DEBUG), "\n"));
				a.logger(DEBUG) << std::endl;
			}

			end_execute(failed.empty());
		}

		void task::handle_execute(bool res)
		{
			CHECK_INTERRUPT

			a.logger(DEBUG) << "[Task " << name << "] Recipe execution finish ";
			a.logger(DEBUG) << (res ? "with success" : "with failure") << std::endl;
			if (!res)
				return end_execute(res); // XXX Add the error scope

			// check the post-conditions to be sure that everything is ok
			return async_evaluate_postconditions(boost::bind(
						&task::handle_final_postcondition_handle,
						this, _1));
		}

		void task::async_evaluate_recipe_preconditions(conditionV failed, size_t i)
		{
			CHECK_INTERRUPT

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

			a.logger(DEBUG) << "[Task " << name << "] Finish to evaluate recipe preconditions" << std::endl;

			// time to select a recipe and execute it :)
			std::sort(recipe_states.begin(), recipe_states.end());
			if (! recipe_states[0].failed.empty() &&
				! recipe_states[0].missing_agents.empty() ) {
				a.logger(DEBUG) << "[Task " << name << "] No recipe candidate found" << std::endl;
				return end_execute(false);
			}

			a.logger(DEBUG) << "[Task " << name << "] Choose to execute ";
			a.logger(DEBUG) << recipes[recipe_states[0].index]->r_name() << std::endl;
			executing_recipe = true;
			return recipes[recipe_states[0].index]->execute(
					boost::bind(&task::handle_execute, this, _1));
		}

		void task::handle_precondition_handle(conditionV failed)
		{
			CHECK_INTERRUPT

			/* If some pre-conditions are not ok, something is wrong, just
			 * returns false */
			a.logger(DEBUG) << "[Task " << name << "] Pre-condition evaluated" << std::endl;

			if (must_interrupt)
				return end_execute(false);

			if (!failed.empty()) {
				a.logger(DEBUG) << "[Task " << name << "] Failure of some pre-conditions ";
				std::copy(failed.begin(), failed.end(),
						  std::ostream_iterator<std::string>(a.logger(DEBUG), ", "));
				a.logger(DEBUG) << std::endl;
				return end_execute(false);
			}

			if (recipes.empty()) {
				a.logger(DEBUG) << "[Task " << name << "] No recipe to handle it" << std::endl;
				return end_execute(false);
			}

			a.logger(DEBUG) << "Current known alive agents ";
			std::copy(a.alive_agents.begin(), a.alive_agents.end(), 
					 std::ostream_iterator<std::string>(a.logger(DEBUG), ", "));
			a.logger(DEBUG) << std::endl;
			
			boost::optional<size_t> first_electable;
			/* Evaluate the computable recipes, depending on the availables agent */
			for (size_t i = 0; i < recipes.size(); ++i) {
				recipe_states[i].index = i;
				recipe_states[i].missing_agents.clear();
				std::set_difference(recipes[i]->begin(), recipes[i]->end(), 
									a.alive_agents.begin(), a.alive_agents.end(),
									std::inserter(recipe_states[i].missing_agents, 
												  recipe_states[i].missing_agents.end()));

				if (!recipe_states[i].missing_agents.empty()) {
					a.logger(DEBUG) << "[Task " << name << "] Won't evaluate " << recipes[i]->r_name();
					a.logger(DEBUG) << " because the following required agents are not available : ";
					std::copy(recipe_states[i].missing_agents.begin(),
							  recipe_states[i].missing_agents.end(),
							  std::ostream_iterator<std::string>(a.logger(DEBUG), ", "));
					a.logger(DEBUG) << std::endl;
				}

				if (recipe_states[i].missing_agents.empty() && !first_electable) 
					first_electable = i;
				
			}

			if (!first_electable) {// no computable recipe
				a.logger(DEBUG) << "[Task " << name << "] No computable recipe" << std::endl;
				return end_execute(false);
			}

			a.logger(DEBUG) << "[Task " << name << "] Starting to evaluate recipe preconditions" << std::endl;
			/* compute precondition for all recipe */
			recipes[*first_electable]->async_evaluate_preconditions(
					boost::bind(&task::async_evaluate_recipe_preconditions,
								this, _1, 0));
		}

		void task::handle_initial_postcondition_handle(conditionV failed)
		{
			CHECK_INTERRUPT
			/* If all post-conditions are ok, no need to execute the task
			 * anymore */
			if (failed.empty()) {
				a.logger(DEBUG) << "[Task " << name << "] All post-conditions already OK " << std::endl;
				return end_execute(true);
			}

			if (must_interrupt) 
				return end_execute(false);

			a.logger(DEBUG) << "[Task " << name << "] Evaluation pre-condition" << std::endl;
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
			must_interrupt = false;
			executing_recipe = false;

			a.logger(DEBUG) << "[Task " << name <<"] Start execution " << std::endl;

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

		void task::abort()
		{
			must_interrupt = true;
			if (executing_recipe)
				recipes[recipe_states[0].index]->abort();
		}

		void task::end_execute(bool res)
		{
			is_running = false;

			a.logger(DEBUG) << " [Task " << name << "] Finish execution ";
			a.logger(DEBUG) << (res ? " with success" : " with failure") << std::endl;

			std::for_each(pending_cb.begin(), pending_cb.end(),
					callback(a, res));

			pending_cb.clear();
		}

		void task::add_recipe(recipe_ptr ptr)
		{
			recipes.push_back(ptr);
			recipe_states.resize(recipes.size());
		}
#undef CHECK_INTERRUPT
	}
}
