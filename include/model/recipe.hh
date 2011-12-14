#ifndef HYPER_MODEL_RECIPE_HH_
#define HYPER_MODEL_RECIPE_HH_

#include <vector>
#include <set>

#include <boost/optional.hpp>

#include <model/abortable_function.hh>
#include <model/task_fwd.hh>
#include <model/recipe_fwd.hh>

namespace hyper {
	namespace model {
		struct ability;
		struct abortable_computation;
		struct task;

		class recipe {
			private:
				std::string name;
				ability &a;
				task& t;
				boost::optional<logic::expression> expected_error_;

				bool is_running;
				bool must_pause;
				bool must_interrupt;
				std::vector<recipe_execution_callback> pending_cb;

				void handle_execute(const boost::system::error_code& e);
				void end_execute(bool res);

			protected:
				abortable_computation* computation;
				virtual void do_execute(abortable_computation::cb_type cb, bool must_pause) = 0;

				/** list of agents required to use this recipe */
				std::set<std::string> required_agents; 

				/** list of logic::expression which appears in make / ensure in the recipe */
				std::set<logic::expression> constraint_domain;

				/** allow task to access required_agents / constraint_domain*/
				friend struct task;

			public:
				typedef std::set<std::string>::const_iterator agent_const_iterator;
				typedef std::set<logic::expression>::const_iterator constraint_const_iterator;

				recipe(const std::string& name, ability& a, task& t,
					   boost::optional<logic::expression> expected_error = boost::none);
				void execute(recipe_execution_callback cb);
				virtual void async_evaluate_preconditions(condition_execution_callback cb) = 0;
				virtual size_t nb_preconditions() const = 0;

				agent_const_iterator begin() const { return required_agents.begin(); }
				agent_const_iterator end() const { return required_agents.end(); }

				constraint_const_iterator constraint_begin() const { return constraint_domain.begin(); }
				constraint_const_iterator constraint_end() const { return constraint_domain.end(); }

				const std::string& r_name() const { return name; }

				const boost::optional<logic::expression>& expected_error() const 
				{ return expected_error_; }

				void abort();
				void pause();
				void resume();

				virtual ~recipe() {};
		};
	}
}

#endif /* HYPER_MODEL_RECIPE_HH_ */
