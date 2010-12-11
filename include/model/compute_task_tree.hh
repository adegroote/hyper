#ifndef HYPER_MODEL_COMPUTE_TASK_TREE
#define HYPER_MODEL_COMPUTE_TASK_TREE

#include <deque>
#include <string>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <model/task.hh> // conditionV definition

namespace hyper {
	namespace model {
		struct logic_layer;
		struct logic_context;

		struct async_eval_all_preconditions;

		struct task_logic_evaluation;

		struct cond_logic_evaluation 
		{
			std::string condition;
			std::deque<boost::shared_ptr<task_logic_evaluation> > tasks;

			cond_logic_evaluation() {}
			cond_logic_evaluation(const std::string& condition) : condition(condition) {}

			bool all_tasks_evaluated() const;
		};

		struct task_logic_evaluation
		{
			std::string name;
			bool cond_evaluated;
			std::vector<cond_logic_evaluation> conds;

			task_logic_evaluation() : cond_evaluated(false) {}
			task_logic_evaluation(const std::string& name) : name(name), cond_evaluated(false) {}
		};

		/*
		 * Asynchronously evaluate a task tree
		 *
		 * On the top of an initial constraint, or a specific task, compute
		 * the tree of task needed to enforce this contraint / allow the
		 * execution of the task.
		 *
		 * It returns true in the callback if it finds a combinaison of task,
		 * false otherwise
		 *
		 * XXX The interface is still buggy, and the implementation partial
		 * (there are no terminaison guaranty at the moment), but most of the
		 * needed stuff is here
		 */
		class compute_task_tree {
			private:
				logic_layer& layer;
				logic_context& ctx;
				cond_logic_evaluation cond_root;

			friend struct async_eval_all_preconditions;

			public:
				typedef boost::function<void (bool)> cb_type;

			public:
				compute_task_tree(logic_layer& layer_, logic_context& ctx_); 
				void async_eval_task(const std::string& s, cb_type);
				void async_eval_cond(const std::string& s, cb_type);

			private:
				void handle_eval_all_constraints(cond_logic_evaluation&, 
						bool res, cb_type handler);
				void async_eval_constraint(cond_logic_evaluation&, cb_type);
				void handle_eval_constraint(task_logic_evaluation& task, 
											  size_t i, bool res,  cb_type );
				void handle_eval_all_preconditions(cond_logic_evaluation&,
					task_logic_evaluation&, conditionV, cb_type);
				void start_eval_constraint(task_logic_evaluation&, cb_type );
				void async_evaluate_preconditions(task_logic_evaluation&, 
												 condition_execution_callback );
		};
	}
}

#endif
