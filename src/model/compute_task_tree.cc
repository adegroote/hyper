#include <algorithm>
#include <numeric>

#include <model/ability.hh>
#include <model/compute_task_tree.hh>
#include <model/logic_layer.hh>
#include <model/execute_impl.hh>
#include <model/task.hh>

namespace {
	using namespace hyper::model;

	struct class_task_evaluation
	{
		bool operator() (const boost::shared_ptr<task_logic_evaluation>& t1, 
						 const boost::shared_ptr<task_logic_evaluation>& t2)
		{
			return (t1->conds.size() < t2->conds.size());
		}
	};

	struct generate_task_eval {
		const cond_logic_evaluation& cond;

		generate_task_eval(const cond_logic_evaluation& cond): cond(cond) {}

		boost::shared_ptr<task_logic_evaluation> 
		operator() (const std::string& name) const
		{
			return boost::make_shared<task_logic_evaluation>(name, cond);
		}
	};

	struct fill_used_task {
		const task_logic_evaluation& task;

		fill_used_task(const task_logic_evaluation& task): task(task) {}

		void operator()(cond_logic_evaluation& cond) {
			cond.task_used = task.task_used;
		}
	};

	struct eval_task_evaluation
	{
		bool operator()(bool b, boost::shared_ptr<task_logic_evaluation> t) const
		{
			return (b && t->cond_evaluated);
		}
	};

	struct eval_task_execution
	{
		bool operator()(bool b, boost::shared_ptr<task_logic_evaluation> t) const
		{
			return (b && t->res_exec);
		}
	};

	struct eval_success_execution
	{
		bool operator()(bool b, boost::shared_ptr<task_logic_evaluation> t) const
		{
			return (b && *t->res_exec);
		}
	};

	struct eval_condition_execution 
	{
		bool operator()(bool b, const cond_logic_evaluation& cond)
		{
			return (b && cond.all_tasks_executed());
		}
	};

	struct eval_success_condition_execution 
	{
		bool operator()(bool b, const cond_logic_evaluation& cond)
		{
			return (b && cond.is_succesfully_executed());
		}
	};
}

namespace hyper {
	namespace model {
		bool cond_logic_evaluation::all_tasks_evaluated() const
		{
			return std::accumulate(tasks.begin(), tasks.end(), true,
					eval_task_evaluation());
		}

		bool cond_logic_evaluation::all_tasks_executed() const
		{
			return std::accumulate(tasks.begin(), tasks.end(), true,
					eval_task_execution());
		}

		bool cond_logic_evaluation::is_succesfully_executed() const
		{
			return std::accumulate(tasks.begin(), tasks.end(), true,
					eval_success_execution());
		}

		bool task_logic_evaluation::all_conds_executed() const
		{
			return std::accumulate(conds.begin(), conds.end(), true,
					eval_condition_execution());
		}

		bool task_logic_evaluation::all_conds_succesfully_executed() const
		{
			return std::accumulate(conds.begin(), conds.end(), true,
					eval_success_condition_execution());
		}

		compute_task_tree::compute_task_tree(logic_layer& layer_,  logic_context& ctx_) :
			layer(layer_), ctx(ctx_), must_interrupt(false), must_pause(false), resume_handler(boost::none)
		{}

#define CHECK_INTERRUPT if (must_interrupt) return handler(false);

		void 
		compute_task_tree::handle_eval_all_constraints(
				cond_logic_evaluation& cond, bool res, compute_task_tree::cb_type handler)
		{
			CHECK_INTERRUPT

			if (res) {
				/* 
				 * remove the not evaluated task tree, if it fails, we just
				 * recompute it without the offending task, so it won't change
				 * too much the behaviour 
				 */
				cond.tasks.erase(cond.tasks.begin() + 1, cond.tasks.end());
				return handler(res);
			} else {
				cond.tasks.pop_front();
				if (cond.tasks.empty())
					return handler(false);

				start_eval_constraint(*(cond.tasks[0]), 
						boost::bind(&compute_task_tree::handle_eval_all_constraints, 
							this, boost::ref(cond), _1, handler));
			}
		}

		void compute_task_tree::handle_eval_all_preconditions(
				cond_logic_evaluation& cond, task_logic_evaluation& task,
				const boost::system::error_code& e,
				conditionV failed, compute_task_tree::cb_type handler)
		{
			CHECK_INTERRUPT 

			layer.a_.logger(DEBUG) << ctx.ctr << " End evaluation precondition for task ";
			layer.a_.logger(DEBUG) << task.name << " " << e << std::endl;

			if (e)
				handler(false);

			task.conds.resize(failed.size());
			std::copy(failed.begin(), failed.end(), task.conds.begin());
			std::for_each(task.conds.begin(), task.conds.end(), fill_used_task(task));
			task.cond_evaluated = true;

			if (!cond.all_tasks_evaluated()) {
				return;
			}

			std::sort(cond.tasks.begin(), cond.tasks.end(), class_task_evaluation());

			start_eval_constraint(*(cond.tasks[0]), 
					boost::bind(&compute_task_tree::handle_eval_all_constraints, 
						this, boost::ref(cond), _1, handler));
		}

		struct async_eval_all_preconditions
		{
			compute_task_tree& tree;
			compute_task_tree::cb_type handler;
			cond_logic_evaluation& cond;

			async_eval_all_preconditions(compute_task_tree& tree, 
					compute_task_tree::cb_type handler,
					cond_logic_evaluation& cond) :
				tree(tree), handler(handler), cond(cond)
			{}

			void operator() (boost::shared_ptr<task_logic_evaluation> task_eval)  const
			{
				tree.async_evaluate_preconditions(*task_eval, 
						boost::bind(&compute_task_tree::handle_eval_all_preconditions, 
							&tree, boost::ref(cond), boost::ref(*task_eval), 
							_1, _2, handler));
			}
		};

		void compute_task_tree::handle_evaluate_hypothesis(size_t i, size_t j, cond_logic_evaluation& cond,
																			   compute_task_tree::cb_type handler)
		{
			CHECK_INTERRUPT

			size_t next_i, next_j;
			bool no_more = false;

			boost::optional<bool> b = hyp_eval[i].res_exec;
			if (b && *b) {
				hyp_eval[i].any_true = true;
				if (i + 1 == hyp_eval.size())
					no_more = true;
				else {
					next_i = i + 1;
					next_j = 0;
				}
			}

			if (j + 1 == hyp_eval[i].hyps.hyps.size()) {
				if (i + 1 == hyp_eval.size()) {
					no_more = true;
				} else {
					next_i = i + 1;
					next_j = 0;
				}
			} else {
				next_i = i;
				next_j = j + 1;
			}


			if (no_more) {
				std::vector<std::string> res;
				for (size_t k = 0; k < hyp_eval.size(); ++k)
					if (hyp_eval[k].any_true)
						res.push_back(hyp_eval[k].hyps.name);

				if (res.empty())
					return handler(false);
				else {
					std::transform(res.begin(), res.end(), std::back_inserter(cond.tasks),
							generate_task_eval(cond));

					std::for_each(cond.tasks.begin(), cond.tasks.end(),
							async_eval_all_preconditions(*this, handler, cond));
				}
			} else
				async_evaluate_hypothesis(next_i, next_j, cond, handler);
		}

		void 
		compute_task_tree::async_evaluate_hypothesis(size_t i, size_t j, cond_logic_evaluation& cond, 
																		 compute_task_tree::cb_type handler)
		{
			CHECK_INTERRUPT

			layer.a_.logger(DEBUG) << ctx.ctr << " Evaluating hypothesis " << hyp_eval[i].hyps.hyps[j] << std::endl;
			return async_eval_expression(layer.a_.io_s, hyp_eval[i].hyps.hyps[j],
										  layer.a_, hyp_eval[i].res_exec,
										  hyp_eval[i].err_ctx,
										  boost::bind(&compute_task_tree::handle_evaluate_hypothesis, this, i, j, 
													   boost::ref(cond), handler));
		}

		void 
		compute_task_tree::async_eval_constraint(cond_logic_evaluation& cond, 
												 compute_task_tree::cb_type handler)
		{
			CHECK_INTERRUPT

			layer.a_.logger(DEBUG) << ctx.ctr << " Searching to solve ";
			layer.a_.logger(DEBUG) << cond.condition << std::endl;
			std::vector<std::string> res;
			std::vector<logic::engine::plausible_hypothesis> hyps;

			std::set<std::string> unusable_tasks(failed_tasks.begin(), failed_tasks.end());
			unusable_tasks.insert(cond.task_used.begin(), cond.task_used.end());

			layer.engine.infer_all_in(cond.condition, std::back_inserter(res), std::back_inserter(hyps),
									  unusable_tasks.begin(), unusable_tasks.end());

			if (res.empty() && hyps.empty())
				return handler(false);

			if (!res.empty()) {
				cond.tasks.clear();
				std::transform(res.begin(), res.end(), std::back_inserter(cond.tasks),
						generate_task_eval(cond));

				std::for_each(cond.tasks.begin(), cond.tasks.end(),
						async_eval_all_preconditions(*this, handler, cond));
			} else {
				hyp_eval.clear();
				std::copy(hyps.begin(), hyps.end(), std::back_inserter(hyp_eval));

				async_evaluate_hypothesis(0, 0, cond, handler);
			}
		}

		void 
		compute_task_tree::handle_eval_constraint(task_logic_evaluation& task, 
				size_t i, bool res, compute_task_tree::cb_type handler)
		{
			CHECK_INTERRUPT

			if (!res) {
				layer.a_.logger(DEBUG) << ctx.ctr << " failed to solve ";
				layer.a_.logger(DEBUG) << task.conds[i].condition << std::endl;
				handler(false);
			} else {
				layer.a_.logger(DEBUG) << ctx.ctr << " Has solved ";
				layer.a_.logger(DEBUG) << task.conds[i].condition << std::endl;

				if (i+1 == task.conds.size()) {

					layer.a_.logger(DEBUG) << ctx.ctr << " Deal with all preconds of ";
					layer.a_.logger(DEBUG) << task.name  << std::endl;

					handler(true);
				} else {
					async_eval_constraint(task.conds[i+1], 
							boost::bind(&compute_task_tree::handle_eval_constraint, 
								this, boost::ref(task), i+1, _1, handler));
				}
			}
		}

		void
		compute_task_tree::start_eval_constraint(task_logic_evaluation& task, 
												 compute_task_tree::cb_type handler)
		{
			CHECK_INTERRUPT

			if (task.conds.empty())
				handler(true);
			else {
				async_eval_constraint(task.conds[0], 
						boost::bind(&compute_task_tree::handle_eval_constraint, 
							this, boost::ref(task), 0,  _1, handler));
			}
		}

		void 
		compute_task_tree::async_evaluate_preconditions(task_logic_evaluation& task, 
														condition_execution_callback handler)
		{
			layer.a_.logger(DEBUG) << ctx.ctr;
			layer.a_.logger(DEBUG) << " Start evaluation precondition for task " << task.name;
			layer.a_.logger(DEBUG) << std::endl;

			layer.tasks[task.name]->async_evaluate_preconditions(handler);
		};

		void
		compute_task_tree::async_eval_task(const std::string& s, 
										   compute_task_tree::cb_type  handler)
		{
			must_interrupt = false;
			running_tasks.clear();
			cond_root = cond_logic_evaluation(logic::function_call());
			cond_root.tasks.push_back(generate_task_eval(cond_root)(s));
			async_eval_all_preconditions(*this, handler, cond_root)(cond_root.tasks[0]);
		}

		void 
		compute_task_tree::async_eval_cond(const logic::function_call& f,
										   compute_task_tree::cb_type handler)
		{
			must_interrupt = false;
			running_tasks.clear();
			cond_root = cond_logic_evaluation(f);
			async_eval_constraint(cond_root, handler);
		}

		void compute_task_tree::handle_execute_cond(task_logic_evaluation& task,
													compute_task_tree::cb_type handler,
													bool inform)
		{
			CHECK_INTERRUPT

			if (task.all_conds_executed()) {
				if (task.all_conds_succesfully_executed()) {
					running_tasks.insert(task.name);
					if (must_pause)
						layer.tasks[task.name]->pause();

					if (inform)
						ctx.ctr.s = network::request_constraint_answer::RUNNING;
					layer.tasks[task.name]->execute(ctx.ctr, handler);
				} else
					handler(false);
			}
		}

		void compute_task_tree::handle_execute_task(cond_logic_evaluation& cond,
													task_logic_evaluation& task,
													bool res,
													cb_type handler)
		{
			CHECK_INTERRUPT

			/* copy the error context from the task into the main logic_layer
			 * error context */
			const hyper::network::error_context& task_err_ctx =
				layer.tasks[task.name]->get_error_context();
			ctx.err_ctx.insert(ctx.err_ctx.end(), 
					task_err_ctx.begin(), task_err_ctx.end());

			running_tasks.erase(task.name);
			task.res_exec = res;

			if (!res) 
				failed_tasks.push_back(task.name);
			
			if (cond.all_tasks_executed()) {
				handler(cond.is_succesfully_executed());
			}
		}

		struct async_exec_all_conditions
		{
			compute_task_tree& tree;
			compute_task_tree::cb_type handler;
			task_logic_evaluation& task;
			bool inform;

			async_exec_all_conditions(compute_task_tree& tree, 
					compute_task_tree::cb_type handler,
					task_logic_evaluation& task,
					bool inform):
				tree(tree), handler(handler), task(task), inform(inform)
			{}

			void operator() (cond_logic_evaluation& cond)  const
			{
				tree.async_execute_cond(cond, 
						boost::bind(&compute_task_tree::handle_execute_cond, 
							&tree, boost::ref(task), handler, inform), false);
			}
		};

		void compute_task_tree::async_execute_task(task_logic_evaluation& task,
												   compute_task_tree::cb_type handler,
												   bool inform)
		{
			CHECK_INTERRUPT

			if (task.conds.empty()) {
				running_tasks.insert(task.name);
				if (must_pause)
					layer.tasks[task.name]->pause();

				if (inform)
					ctx.ctr.s = network::request_constraint_answer::RUNNING;

				layer.tasks[task.name]->execute(ctx.ctr, handler);
			} else {
				std::for_each(task.conds.begin(), task.conds.end(), 
							  async_exec_all_conditions(*this, handler, task, inform));
			}
		}

		struct async_exec_all_tasks
		{
			compute_task_tree& tree;
			compute_task_tree::cb_type handler;
			cond_logic_evaluation& cond;
			bool inform;

			async_exec_all_tasks(compute_task_tree& tree, 
					compute_task_tree::cb_type handler,
					cond_logic_evaluation& cond,
					bool inform):
				tree(tree), handler(handler), cond(cond), inform(inform)
			{}

			void operator() (boost::shared_ptr<task_logic_evaluation> task_eval)  const
			{
				tree.async_execute_task(*task_eval, 
						boost::bind(&compute_task_tree::handle_execute_task, 
							&tree, boost::ref(cond), boost::ref(*task_eval), 
							_1, handler), inform);
			}
		};

		void
		compute_task_tree::async_execute_cond(cond_logic_evaluation& cond,
											  compute_task_tree::cb_type handler,
											  bool inform)
		{
			CHECK_INTERRUPT

			if (cond.all_tasks_executed()) {
				handler(cond.is_succesfully_executed());
			} else {
				std::for_each(cond.tasks.begin(), cond.tasks.end(), 
						async_exec_all_tasks(*this, handler, cond, inform));
			}
		}

		void
		compute_task_tree::async_execute(compute_task_tree::cb_type handler)
		{
			running_tasks.clear();

			if (must_pause) {
				resume_handler = boost::bind(&compute_task_tree::async_execute, 
										 this, handler);
			} else {
				must_interrupt = false;
				bool inform = (cond_root.condition != logic::function_call());
				async_execute_cond(cond_root, handler, inform);
			}
		}

		struct exec_ {
			typedef boost::function<void (task_ptr)> fun;

			logic_layer& layer;
			fun f;
			
			exec_(logic_layer &layer, fun f) : layer(layer), f(f) {}

			void operator() (const std::string& name) 
			{
				f(layer.tasks[name]);
			}
		};

		void compute_task_tree::abort()
		{
			must_interrupt = true;
			std::for_each(running_tasks.begin(), running_tasks.end(), 
					exec_(layer, boost::bind(&task::abort, _1)));
		}

		void compute_task_tree::pause()
		{
			must_pause = true;
			std::for_each(running_tasks.begin(), running_tasks.end(), 
						  exec_(layer, boost::bind(&task::pause, _1)));
		}

		void compute_task_tree::resume()
		{
			must_pause = false;
			std::for_each(running_tasks.begin(), running_tasks.end(), 
						  exec_(layer, boost::bind(&task::resume, _1)));
			if (resume_handler) 
				(*resume_handler)();
			resume_handler = boost::none;
		}
	}
}
