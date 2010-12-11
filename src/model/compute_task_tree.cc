#include <algorithm>
#include <numeric>

#include <model/ability.hh>
#include <model/compute_task_tree.hh>
#include <model/logic_layer.hh>

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
		boost::shared_ptr<task_logic_evaluation> 
		operator() (const std::string& name) const
		{
			return boost::make_shared<task_logic_evaluation>(name);
		}
	};

	struct eval_task_evaluation
	{
		bool operator()(bool b, boost::shared_ptr<task_logic_evaluation> t) const
		{
			return (b && t->cond_evaluated);
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

		compute_task_tree::compute_task_tree(logic_layer& layer_,  logic_context& ctx_) :
			layer(layer_), ctx(ctx_)
		{}

		void 
		compute_task_tree::handle_eval_all_constraints(
				cond_logic_evaluation& cond, bool res, compute_task_tree::cb_type handler)
		{
			if (res) {
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
				conditionV failed, compute_task_tree::cb_type handler)
		{
			layer.a_.logger(DEBUG) << ctx.ctr << " End evaluation precondition for task ";
			layer.a_.logger(DEBUG) << task.name << std::endl;

			task.conds.resize(failed.size());
			std::copy(failed.begin(), failed.end(), task.conds.begin());
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
							_1, handler));
			}
		};

		void 
		compute_task_tree::async_eval_constraint(cond_logic_evaluation& cond, 
												 compute_task_tree::cb_type handler)
		{
			layer.a_.logger(DEBUG) << ctx.ctr << " Searching to solve ";
			layer.a_.logger(DEBUG) << cond.condition << std::endl;
			std::vector<std::string> res;
			layer.engine.infer(cond.condition, std::back_inserter(res));

			if (res.size() == 0)
				handler(false);

			std::transform(res.begin(), res.end(), std::back_inserter(cond.tasks),
					generate_task_eval());

			std::for_each(cond.tasks.begin(), cond.tasks.end(),
					async_eval_all_preconditions(*this, handler, cond));
		}

		void 
		compute_task_tree::handle_eval_constraint(task_logic_evaluation& task, 
				size_t i, bool res, compute_task_tree::cb_type handler)
		{
			if (!res) {
				layer.a_.logger(DEBUG) << ctx.ctr << " Failed to solve ";
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
			cond_root.condition = "";
			cond_root.tasks.push_back(generate_task_eval()(s));
			async_eval_all_preconditions(*this, handler, cond_root)(cond_root.tasks[0]);
		}

		void 
		compute_task_tree::async_eval_cond(const std::string& s, 
										   compute_task_tree::cb_type handler)
		{
			cond_root.condition = s;
			async_eval_constraint(cond_root, handler);
		}
	}
}
