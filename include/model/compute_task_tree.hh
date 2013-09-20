#ifndef HYPER_MODEL_COMPUTE_TASK_TREE
#define HYPER_MODEL_COMPUTE_TASK_TREE

#include <deque>
#include <string>

#include <boost/function/function0.hpp>
#include <boost/function/function1.hpp>
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
			logic::function_call condition; /**< the condition to fullfil */
			std::deque<boost::shared_ptr<task_logic_evaluation> > tasks; /**<
					task_logic_evaluation which can fullfill #condition */
			std::set<std::string> task_used; /**< tasks used so far to achieve this state */

			cond_logic_evaluation() {}
			cond_logic_evaluation(const logic::function_call& condition) : condition(condition) {}

			bool all_tasks_evaluated() const;
			bool all_tasks_executed() const;
			bool is_succesfully_executed() const;
		};

		struct task_logic_evaluation
		{
			std::string name; /**< task name */
			bool cond_evaluated; /**< has evaluated all this missing condition */
			std::vector<cond_logic_evaluation> conds; /**< its requiered condition */
			boost::optional<bool> res_exec; /**< execution state */
			std::set<std::string> task_used; /**< tasks used so far to achieve this state */

			task_logic_evaluation() : cond_evaluated(false), res_exec(boost::none) {}
			task_logic_evaluation(const std::string& name, const cond_logic_evaluation& cond) : 
				name(name), cond_evaluated(false), res_exec(boost::none),
				task_used(cond.task_used)
			{
				task_used.insert(name);
			}

			bool all_conds_executed() const;
			bool all_conds_succesfully_executed() const;
		};

		struct hypothesis_evaluation
		{
			bool any_true; /**< check if any hypothesis is true */
			logic::engine::plausible_hypothesis hyps; /**< all the hypothesis */
			boost::optional<bool> res_exec; /**< evaluation of one hypothesis */
			hyper::network::error_context err_ctx; /** < error_context related */

			hypothesis_evaluation(const logic::engine::plausible_hypothesis& h) :
				any_true(false), hyps(h), res_exec(boost::none)
			{}
		};

		/**
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
		 */
		class compute_task_tree {
			private:
				logic_layer& layer;
				logic_context& ctx;
				cond_logic_evaluation cond_root;
				std::vector<hypothesis_evaluation> hyp_eval;

				bool must_interrupt;
				bool must_pause;
				typedef boost::function<void (void)> resume_fun;
				boost::optional<resume_fun> resume_handler;

				std::set<std::string> running_tasks;

				std::vector<std::string> failed_tasks;

			friend struct async_eval_all_preconditions;
			friend struct async_exec_all_tasks;
			friend struct async_exec_all_conditions;

			public:
				typedef boost::function<void (bool)> cb_type;

			public:
				/**
				 * The constructor
				 *
				 * @param layer : a reference to the associated logic layer, in
				 * particular to access to different tasks
				 * @param ctx : the context of the logic constraint we need to
				 * take care about 
				 * */
				compute_task_tree(logic_layer& layer_, logic_context& ctx_); 

				/**
				 * compute the task tree requiered for a specific task
				 *
				 * @param s : the name of the task
				 * @param cb : the callback to use when computed
				 *
				 * @return nothing. Update #cond_root and its associated tree
				 * in success case.
				 */
				void async_eval_task(const std::string& s, cb_type cb);

				/**
				 * compute the task tree requiered for a specific condition
				 *
				 * @param f : the origin fact we need to resolve
				 * @param cb : the callback to use when computed
				 *
				 * @return nothing. Update #cond_root and its associated tree
				 * in success case.
				 */
				void async_eval_cond(const logic::function_call& f, cb_type cb);

				/**
				 * execute a computed tree. The caller must be sure that there
				 * is a valid tree, i.e. that async_eval_task() or
				 * async_eval_cond() has terminated with success.
				 *
				 * @param cb : the callback to use when the execution is terminated.
				 */
				void async_execute(cb_type cb);

				/**
				 * abort the computation or execution of a task tree. The
				 * callback associated to the active method will return an
				 * interrupted error.
				 */
				void abort();

				/**
				 * pause the execution of a task tree. No effect if object is 
				 * doing nothing or is computing the tree.
				 * */
				void pause();

				/**
				 * resume the execution of a task tree. No effect if the object is 
				 * doing nothing or is computing the tree.
				 */
				void resume();

			private:

				/*
				 * Relation between the different methods. handle_... are
				 * related to their associed async_... counterpart
				 *
				 * async_eval_cond -> async_eval_constraint 
				 * async_eval_constraint -> infer_all_in -> fill cond.tasks
				 * \foreach task in cond.tasks -> async_eval_precondition task 
				 *							   -> handle_eval_precondition
				 *							   -> fill task.conds
				 * \foreach cond in task.conds -> async_eval_constraint cond
				 */
				void handle_eval_all_constraints(cond_logic_evaluation&, 
						bool res, cb_type handler);
				void handle_eval_all_preconditions(cond_logic_evaluation&,
					task_logic_evaluation&, const boost::system::error_code&,
					conditionV, cb_type);

				void async_eval_constraint(cond_logic_evaluation& cond, cb_type cb);

				void start_eval_constraint(task_logic_evaluation& task, cb_type cb);
				void handle_eval_constraint(task_logic_evaluation& task, 
											  size_t i, bool res,  cb_type cb);

				void async_evaluate_preconditions(task_logic_evaluation&, 
												 condition_execution_callback );

				void async_execute_cond(cond_logic_evaluation& cond, cb_type, bool);
				void async_execute_task(task_logic_evaluation& task, cb_type, bool);
				void handle_execute_cond(task_logic_evaluation& task, cb_type, bool);
				void handle_execute_task(cond_logic_evaluation& cond,
										 task_logic_evaluation& task,
										 bool res, cb_type handler);

				void async_evaluate_hypothesis(size_t i, size_t j, cond_logic_evaluation&, cb_type);
				void handle_evaluate_hypothesis(size_t, size_t, cond_logic_evaluation&, cb_type);
		};
	}
}

#endif
