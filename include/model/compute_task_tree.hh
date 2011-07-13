#ifndef HYPER_MODEL_COMPUTE_TASK_TREE
#define HYPER_MODEL_COMPUTE_TASK_TREE

#include <deque>
#include <string>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <logic/engine.hh>

#include <model/logic_layer_fwd.hh>
#include <model/task_fwd.hh>

namespace hyper {
	namespace model {

		struct async_eval_all_preconditions;
		struct async_exec_all_tasks;
		struct async_exec_all_conditions;

		struct task_logic_evaluation;

		struct cond_logic_evaluation 
		{
			std::string condition;
			std::deque<boost::shared_ptr<task_logic_evaluation> > tasks;

			cond_logic_evaluation() {}
			cond_logic_evaluation(const std::string& condition) : condition(condition) {}

			bool all_tasks_evaluated() const;
			bool all_tasks_executed() const;
			bool is_succesfully_executed() const;
		};

		struct task_logic_evaluation
		{
			std::string name;
			bool cond_evaluated;
			std::vector<cond_logic_evaluation> conds;
			boost::optional<bool> res_exec;

			task_logic_evaluation() : cond_evaluated(false), res_exec(boost::none) {}
			task_logic_evaluation(const std::string& name) : name(name), cond_evaluated(false),
															 res_exec(boost::none) {}

			bool all_conds_executed() const;
			bool all_conds_succesfully_executed() const;
		};

		struct hypothesis_evaluation
		{
			bool any_true;
			logic::engine::plausible_hypothesis hyps;
			boost::optional<bool> res_exec;

			hypothesis_evaluation(const logic::engine::plausible_hypothesis& h) :
				any_true(false), hyps(h), res_exec(boost::none)
			{}
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
		 * The async_execute just execute the previously tree computed. It is
		 * only valid if it finds a combinaison of tasks. It is probably not
		 * The Right Way to Do, it is probably more efficient to really mix
		 * "evaluation / execution". Will do it later.
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
				std::vector<hypothesis_evaluation> hyp_eval;

				bool must_interrupt;
				std::set<std::string> running_tasks;

			friend struct async_eval_all_preconditions;
			friend struct async_exec_all_tasks;
			friend struct async_exec_all_conditions;

			public:
				typedef boost::function<void (bool)> cb_type;

			public:
				compute_task_tree(logic_layer& layer_, logic_context& ctx_); 
				void async_eval_task(const std::string& s, cb_type);
				void async_eval_cond(const std::string& s, cb_type);

				void async_execute(cb_type);
				void abort();

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

				void async_execute_cond(cond_logic_evaluation& cond, cb_type);
				void async_execute_task(task_logic_evaluation& task, cb_type);
				void handle_execute_cond(task_logic_evaluation& task, cb_type);
				void handle_execute_task(cond_logic_evaluation& cond,
										 task_logic_evaluation& task,
										 bool res, cb_type handler);

				void async_evaluate_hypothesis(size_t i, size_t j, cond_logic_evaluation&, cb_type);
				void handle_evaluate_hypothesis(size_t, size_t, cond_logic_evaluation&, cb_type);
		};
	}
}

#endif
