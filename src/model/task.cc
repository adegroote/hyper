#include <network/log_level.hh>

#include <model/ability.hh>
#include <model/task.hh>
#include <model/recipe.hh>
#include <utils/algorithm.hh>

#include <boost/spirit/home/phoenix.hpp>
#include <boost/fusion/include/std_pair.hpp>

namespace phx = boost::phoenix;

namespace {
	using namespace hyper::model;
	struct callback {
		ability & a;
		bool res;

		callback(ability& a, bool res) : a(a), res(res) {}	

		void operator()(task::ctx_cb c) { 
			a.io_s.post(boost::bind(c.second, res));
		}
	};

	struct run {
		ability & a;

		run(ability&a) : a(a) {}

		void operator()(task::ctx_cb& c) {
			switch (c.first.s) {
				case hyper::network::request_constraint_answer::RUNNING:
				case hyper::network::request_constraint_answer::TEMP_FAILURE:
					c.first.s = hyper::network::request_constraint_answer::RUNNING;
					update_ctr_status(a, c.first);
					break;
				default:
					break;
			}
		}
	};

	struct temp_fail {
		ability & a;

		temp_fail(ability&a) : a(a) {}

		void operator()(task::ctx_cb& c) {
			if (c.first.s == hyper::network::request_constraint_answer::RUNNING) {
				c.first.s = hyper::network::request_constraint_answer::TEMP_FAILURE;
				update_ctr_status(a, c.first);
			}
		}
	};
}

namespace hyper {
	namespace model {

		std::ostream& operator << (std::ostream& oss, const task& t) 
		{
			std::for_each(t.pending_cb.begin(), t.pending_cb.end(),
							oss << phx::at_c<0>(phx::arg_names::arg1));
			oss << "[Task " << t.name << "] ";
			return oss;
		}

#define CHECK_INTERRUPT if (must_interrupt) return end_execute(false);

		void task::handle_final_postcondition_handle(const boost::system::error_code &e, conditionV failed)
		{
			CHECK_INTERRUPT

			if (e)
				return end_execute(false);

			if (!failed.empty()) {
				a.logger(DEBUG) << *this << "Still have some failed postcondition : \n";
				std::copy(failed.begin(), failed.end(),
						  std::ostream_iterator<logic::function_call>(a.logger(DEBUG), "\n"));
				a.logger(DEBUG) << std::endl;
			}

			end_execute(failed.empty());
		}

		void task::handle_execute(boost::optional<hyper::logic::expression> l)
		{
			CHECK_INTERRUPT

			bool res = !l;

			a.logger(DEBUG) << *this << "Recipe execution finish ";
			a.logger(DEBUG) << (res ? "with success" : "with failure") << std::endl;
			if (!res && !must_interrupt) {
				error_context.push_back(*l);
				std::for_each(pending_cb.begin(), pending_cb.end(), temp_fail(a));
				return handle_precondition_handle(boost::system::error_code(),
												  conditionV());
			}

			if (!res && must_interrupt) 
				return end_execute(false);
			
			return end_execute(true);
#if 0
			// check the post-conditions to be sure that everything is ok
			return async_evaluate_postconditions(boost::bind(
						&task::handle_final_postcondition_handle,
						this, _1, _2));
#endif
		}

		void task::async_evaluate_recipe_preconditions(const boost::system::error_code& e, conditionV failed, size_t i)
		{
			CHECK_INTERRUPT

			if (e)
				return end_execute(false);

			recipe_states[i].failed = failed;
			recipe_states[i].index = i;

			if (i != (recipes.size() - 1)) {
				boost::optional<size_t> electable;
				for (size_t j = i+1; j < recipes.size() && !electable; ++j)
					if (recipe_states[j].is_electable())
						electable = j;

				if (electable) {
					a.logger(DEBUG) << *this << "Starting to evaluate preconditions for recipe ";
					a.logger(DEBUG) << recipes[*electable]->r_name() << std::endl;
					return recipes[*electable]->async_evaluate_preconditions(
							boost::bind(&task::async_evaluate_recipe_preconditions,
								this, _1, _2, *electable));
				}
			} 

			a.logger(DEBUG) << *this << "Finish to evaluate recipe preconditions" << std::endl;


			// time to select a recipe and execute it :)
			std::sort(recipe_states.begin(), recipe_states.end());
			if (! recipe_states[0].failed.empty() ||
				! recipe_states[0].is_electable()) {
				a.logger(DEBUG) << *this << "No recipe candidate found" << std::endl;
				return end_execute(false);
			}

			a.logger(DEBUG) << *this << "Choose to execute ";
			a.logger(DEBUG) << recipes[recipe_states[0].index]->r_name() << std::endl;
			executing_recipe = true;
			if (must_pause) 
				recipes[recipe_states[0].index]->pause();

			std::for_each(pending_cb.begin(), pending_cb.end(), run(a));

			return recipes[recipe_states[0].index]->execute(
					boost::bind(&task::handle_execute, this, _1));
		}

		void task::handle_precondition_handle(const boost::system::error_code& e, conditionV failed)
		{
			CHECK_INTERRUPT

			if (e)
				return end_execute(false);

			/* If some pre-conditions are not ok, something is wrong, just
			 * returns false */
			a.logger(DEBUG) << *this << "Pre-condition evaluated" << std::endl;

			if (must_interrupt)
				return end_execute(false);

			if (!failed.empty()) {
				a.logger(DEBUG) << *this << "Failure of some pre-conditions ";
				std::copy(failed.begin(), failed.end(),
						  std::ostream_iterator<logic::function_call>(a.logger(DEBUG), ", "));
				a.logger(DEBUG) << std::endl;
				return end_execute(false);
			}

			if (recipes.empty()) {
				a.logger(DEBUG) << *this << "No recipe to handle it" << std::endl;
				return end_execute(false);
			}

			a.logger(DEBUG) << "Current known alive agents ";
			std::copy(a.alive_agents.begin(), a.alive_agents.end(), 
					 std::ostream_iterator<std::string>(a.logger(DEBUG), ", "));
			a.logger(DEBUG) << std::endl;
			
			a.logger(DEBUG) << *this << "Error_context ";
			if (error_context.empty())
				a.logger(DEBUG) << "empty";
			else 
				a.logger(DEBUG) << error_context.back();
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
					a.logger(DEBUG) << *this << "Won't evaluate " << recipes[i]->r_name();
					a.logger(DEBUG) << " because the following required agents are not available : ";
					std::copy(recipe_states[i].missing_agents.begin(),
							  recipe_states[i].missing_agents.end(),
							  std::ostream_iterator<std::string>(a.logger(DEBUG), ", "));
					a.logger(DEBUG) << std::endl;
				}

				/* check if the domain of the recipe contains one of constraint
				 * which appears in the context */
				bool match_domain = hyper::utils::any(error_context.begin(), 
													  error_context.end(),
				phx::count(recipes[i]->constraint_domain, phx::arg_names::arg1) == 1);

				const boost::optional<logic::expression>& expr = recipes[i]->expected_error();
				recipe_states[i].last_error_valid = 
					((!expr && error_context.empty()) ||
					 (!match_domain) ||
					 (expr && !error_context.empty() && error_context.back() == *expr));

				if (!recipe_states[i].last_error_valid) {
					a.logger(DEBUG) << *this << "Won't evaluate " << recipes[i]->r_name();
					a.logger(DEBUG) << " because the error_context is not consistent with expected error ";
					if (!expr)
						a.logger(DEBUG) << "none";
					else
						a.logger(DEBUG) << *expr;
					a.logger(DEBUG) << std::endl;
				}

				if (recipe_states[i].is_electable() && !first_electable) 
					first_electable = i;
			}

			if (!first_electable) {// no computable recipe
				a.logger(DEBUG) << *this << "No computable recipe" << std::endl;
				return end_execute(false);
			}

			a.logger(DEBUG) << *this << "Starting to evaluate preconditions for recipe ";
			a.logger(DEBUG) << recipes[*first_electable]->r_name() << std::endl;
			/* compute precondition for all recipe */
			recipes[*first_electable]->async_evaluate_preconditions(
					boost::bind(&task::async_evaluate_recipe_preconditions,
								this, _1, _2, *first_electable));
		}

		void task::handle_initial_postcondition_handle(const boost::system::error_code& e, conditionV failed)
		{
			if (e) 
				return end_execute(false);

			CHECK_INTERRUPT
			/* If all post-conditions are ok, no need to execute the task
			 * anymore */
			if (failed.empty()) {
				a.logger(DEBUG) << *this << "All post-conditions already OK " << std::endl;
				return end_execute(true);
			}

			if (must_interrupt) 
				return end_execute(false);

			a.logger(DEBUG) << *this << "Evaluation pre-condition" << std::endl;
			return async_evaluate_preconditions(boost::bind(
						&task::handle_precondition_handle, 
						this, _1, _2));
		}

		void task::execute(const logic_constraint& ctr, task_execution_callback cb)
		{
			pending_cb.push_back(std::make_pair(ctr, cb));

			if (executing_recipe && ctr.s == network::request_constraint_answer::RUNNING)
				update_ctr_status(a, ctr);

			if (is_running)
				return;

			is_running = true;
			must_interrupt = false;
			executing_recipe = false;
			error_context.clear();

			a.logger(DEBUG) << *this << "Start execution " << std::endl;

			/* Handle correctly the special case where we don't have
			 * postcondition.  It means that they can't be called using the
			 * logic solver, but that the call has been forced (by an update
			 * stuff for example). In this case, don't report ok now, try to
			 * really execute it */
			if (has_postconditions())
				return async_evaluate_postconditions(boost::bind(
							&task::handle_initial_postcondition_handle, 
							this, _1, _2));
			else
				return async_evaluate_preconditions(boost::bind(
							&task::handle_precondition_handle, 
							this, _1, _2));

		}

		void task::abort()
		{
			must_interrupt = true;
			must_pause = false;
			if (executing_recipe)
				recipes[recipe_states[0].index]->abort();
		}

		void task::pause()
		{
			must_pause = true;
			if (executing_recipe) 
				recipes[recipe_states[0].index]->pause();
		}

		void task::resume()
		{
			must_pause = false;
			if (executing_recipe) 
				recipes[recipe_states[0].index]->resume();
		}

		void task::end_execute(bool res)
		{
			is_running = false;
			executing_recipe = false;
			must_pause = false;

			a.logger(DEBUG) << *this << "Finish execution ";
			a.logger(DEBUG) << (res ? " with success" : " with failure") << std::endl;

			std::for_each(pending_cb.begin(), pending_cb.end(),
					callback(a, res));

			pending_cb.clear();
		}

		void task::add_recipe(recipe_ptr ptr)
		{
			recipes.push_back(ptr);
			recipe_states.resize(recipes.size());
			recipe_states[recipes.size() - 1].nb_preconds = ptr->nb_preconditions();
		}
#undef CHECK_INTERRUPT
	}
}
