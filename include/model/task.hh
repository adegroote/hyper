#ifndef _HYPER_MODEL_TASK_HH_
#define _HYPER_MODEL_TASK_HH_

#include <set>
#include <string>

#include <network/runtime_error.hh>
#include <model/logic_layer_fwd.hh>
#include <model/task_fwd.hh>
#include <model/recipe_fwd.hh>

#include <boost/shared_ptr.hpp>


namespace hyper {
	namespace model {

		struct ability;

		class task
		{
			public:
				typedef std::pair<logic_constraint, task_execution_callback> ctx_cb;

			private:
				ability& a;
				std::string name;
				typedef std::vector<recipe_ptr> vect_recipes;
				vect_recipes recipes;

				struct state {
					size_t index; // index in vect_recipes
					size_t nb_preconds; // nb of preconditions
					size_t size_domain;
					unsigned int preference; // user defined preference, bigger is greater
					std::set<std::string> missing_agents; // missing agent to execute the recipe
					bool last_error_valid; // last_error context coherent with error_context ?
					conditionV failed; // failed precondition

					bool operator< (const state& s) const;

					bool is_electable() const {
						return missing_agents.empty() && last_error_valid;
					}
				};

				std::vector<state> recipe_states;

				bool is_running;
				bool executing_recipe;
				bool must_interrupt;
				bool must_pause;

			private:
				std::vector<ctx_cb> pending_cb;

				friend std::ostream& operator<< (std::ostream&, const task&);

			protected:
				// the full error context 
				std::vector<network::runtime_failure> error_context;

				// the constraint errorc_context, used to not reuse recipe
				// which use the same kind of constraint
				std::vector<logic::expression> constraint_error_context;
				virtual bool has_postconditions() const = 0;

			public:
				task(ability& a_, const std::string& name_) 
					: a(a_), name(name_), is_running(false), executing_recipe(false),
										  must_interrupt(false), must_pause(false) 
				{}

				void add_recipe(recipe_ptr ptr);
				virtual void async_evaluate_preconditions(condition_execution_callback cb) = 0;
				virtual void async_evaluate_postconditions(condition_execution_callback cb) = 0;
				void execute(const logic_constraint& ctr, task_execution_callback cb);
				void abort();
				void pause();
				void resume();
				virtual ~task() {};

				const std::vector<network::runtime_failure>& 
				get_error_context() const {
					return this->error_context;
				}

			private:
				void handle_initial_postcondition_handle(const boost::system::error_code&, conditionV failed);
				void handle_precondition_handle(const boost::system::error_code&, conditionV failed);
				void handle_execute(boost::optional<hyper::network::runtime_failure>);
				void handle_final_postcondition_handle(const boost::system::error_code&, conditionV failed);
				void async_evaluate_recipe_preconditions(const boost::system::error_code&, conditionV, size_t i);
				void end_execute(bool res);
		};
	}
}

#endif /* _HYPER_MODEL_TASK_HH_ */
