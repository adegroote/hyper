#include <logic/expression.hh>
#include <logic/eval.hh>

#include <model/ability.hh>
#include <model/execute_impl.hh>
#include <model/logic_layer_impl.hh>

namespace {

	/* 
	 * Remove % if we found some pattern between % % 
	 * % will be used to denote the type of the expression
	 */
	std::string prepare_execution_rqst(const std::string& s)
	{
		std::string res = s;
		std::string::size_type loc, previous_loc;
		previous_loc = 0;

		while ( (loc = res.find("%", previous_loc)) != std::string::npos)
		{
			previous_loc = loc;
			loc = res.find("%", previous_loc+1);
			if (loc == std::string::npos) 
				break;

			/* We found a pattern between two % */
			res.erase(loc,1);
			res.erase(previous_loc,1);
			previous_loc = loc - 2;
		}

		return res;
	}

	/* 
	 * If we found a pattern between % %, remove it
	 * % will be used to denote the type of the expression, and so is useless
	 * in the logic world
	 */
	std::string prepare_logic_rqst(const std::string& s)
	{
		std::string res = s;
		std::string::size_type loc, previous_loc;
		previous_loc = 0;

		while ( (loc = res.find("%", previous_loc)) != std::string::npos)
		{
			previous_loc = loc;
			loc = res.find("%", previous_loc+1);
			if (loc == std::string::npos) 
				break;

			/* We found a pattern between two % */
			std::string::size_type pattern_size = loc - previous_loc;
			res.erase(previous_loc, pattern_size+1);
		}

		return res;
	}

	void add_transitivy_rule(hyper::logic::engine& e, const std::string& s)
	{
		std::string rule_name = s + "_transitivity";
		std::vector<std::string> cond;
		std::vector<std::string> action;
		
		cond.push_back(s + "(A,B)");
		cond.push_back(s + "(B,C)");
		action.push_back(s + "(A,C)");

		e.add_rule(rule_name, cond, action);
	}

	void add_symetry_rule(hyper::logic::engine& e, const std::string& s)
	{
		std::string rule_name = s + "_symetry";
		std::vector<std::string> cond;
		std::vector<std::string> action;
		
		cond.push_back(s + "(A,B)");

		std::ostringstream oss;
		oss << "equal(" << s << "(A,B), " << s << "(B, A))";
		action.push_back(oss.str());

		e.add_rule(rule_name, cond, action);
	}
}

namespace hyper {
	namespace model {
		logic_layer::logic_layer(ability &a) :
			engine(),
			a_(a)
		{
			/* Add exec func */
			add_equalable_type<int>("int");
			add_equalable_type<double>("double");
			add_equalable_type<std::string>("string");
			add_equalable_type<bool>("bool");

			add_comparable_type<int>("int");
			add_comparable_type<double>("double");
			add_comparable_type<std::string>("string");
			add_comparable_type<bool>("bool");

			add_numeric_type<int>("int");
			add_numeric_type<double>("double");

			/* Add logic func */
			engine.add_predicate("less", 2, new logic::eval<
											details::logic_eval<details::logic_less>, 2>());
			engine.add_predicate("less_equal", 2, new logic::eval<
											details::logic_eval<details::logic_less_equal>, 2>());
			engine.add_predicate("greater", 2, new logic::eval<
											details::logic_eval<details::logic_greater>, 2>());
			engine.add_predicate("greater_equal", 2, new logic::eval<
											details::logic_eval<details::logic_greater_equal>, 2>());

			engine.add_func("add", 2);
			engine.add_func("minus", 2);
			engine.add_func("times", 2);
			engine.add_func("divides", 2);
			engine.add_func("negate", 1);

			/* Add logic rules */
			add_transitivy_rule(engine, "less");
			add_transitivy_rule(engine, "less_equal");
			add_transitivy_rule(engine, "greater");
			add_transitivy_rule(engine, "greater_equal");

			add_symetry_rule(engine, "add");
			add_symetry_rule(engine, "minus");
			add_symetry_rule(engine, "times");
			add_symetry_rule(engine, "divides");
		}

		void logic_layer::async_exec(const logic_constraint& ctr)
		{
			logic_ctx_ptr ctx = boost::make_shared<logic_context>(ctr);

			std::string to_execute = prepare_execution_rqst(ctx->ctr.constraint);
			logic::generate_return ret_exec =
						logic::generate(to_execute, execFuncs);

			if (ret_exec.res == false) {
				a_.logger(WARNING) << ctr << " Fail to parse" << std::endl;
				// XXX
				return;
			}

			ctx->call_exec = ret_exec.e;

			a_.logger(DEBUG) << ctr << " Computation of the state " << std::endl;
			return async_eval_expression(a_.io_s, ctx->call_exec,
										  a_, ctx->exec_res,
				boost::bind(&logic_layer::handle_exec_computation, this,
							boost::asio::placeholders::error,
							ctx));
		}

		struct test_precondition
		{
			logic_layer& layer;
			logic_ctx_ptr ctx;

			test_precondition(logic_layer& layer_, logic_ctx_ptr ctx_) : 
				layer(layer_), ctx(ctx_) {}

			void operator() (const std::string& task)
			{
				layer.a_.logger(DEBUG) << ctx->ctr;
				layer.a_.logger(DEBUG) << " Start evaluation precondition for task " << task;
				layer.a_.logger(DEBUG) << std::endl;
				layer.tasks[task]->async_evaluate_preconditions(
						boost::bind(&logic_layer::handle_evaluation_preconds,
								    &layer, ctx, task, _1));
			}
		};

		void logic_layer::async_exec(const std::string& task, network::identifier id,
									 const std::string& src)

		{
			logic_ctx_ptr ctx = boost::make_shared<logic_context>();
			ctx->ctr.id = id;
			ctx->ctr.src = src;

			ctx->seqs.deps.push_back(task);
			test_precondition(*this, ctx)(ctx->seqs.deps.back().name);
		}

		void logic_layer::handle_exec_computation(const boost::system::error_code& e,
									 logic_ctx_ptr ctx)
		{
			a_.logger(DEBUG) <<  ctx->ctr << " Finish the constraint computation " << std::endl;

			if (e  || !ctx->exec_res) {
				a_.logger(DEBUG) << ctx->ctr << " Failed to evaluate" << std::endl;
				// XXX send a back message to caller
				return;
			}

			if (ctx->exec_res && *(ctx->exec_res)) {
				a_.logger(INFORMATION) << ctx->ctr << " Already enforced" << std::endl;
				return;
			}

			a_.io_s.post(boost::bind(&logic_layer::compute_potential_task,
									 this, ctx));
		}


		void logic_layer::compute_potential_task(logic_ctx_ptr ctx)
		{
			std::string to_logic = prepare_logic_rqst(ctx->ctr.constraint);
			std::vector<std::string> res;

			a_.logger(DEBUG) << ctx-> ctr << " Searching some task to handle it" << std::endl;
			engine.infer(to_logic, std::back_inserter(res));
			a_.logger(DEBUG) << ctx->ctr << " Find " << res.size();
			a_.logger(DEBUG) << " task(s) to handle it" << std::endl;

			if (res.size() == 0) {
				// XXX send back an answer to the agent
				return;
			}

			// compute precondition for each succesful task
			std::copy(res.begin(), res.end(), std::back_inserter(ctx->seqs.deps)); 
			std::for_each(res.begin(), res.end(), test_precondition(*this, ctx));
		}

		struct class_task_evaluation
		{
			bool operator() (const task_evaluation& t1, const task_evaluation& t2)
			{
				return ((*(t1.failed_conds)).size() < (*(t2.failed_conds)).size());
			}
		};

		void logic_layer::handle_evaluation_preconds(logic_ctx_ptr ctx, 
				const std::string& name, conditionV failed)
		{
			a_.logger(DEBUG) << ctx->ctr << " End evaluation precondition for task ";
			a_.logger(DEBUG) << name << std::endl;

			std::vector<task_evaluation>::iterator it;
			std::vector<task_evaluation>& deps = ctx->seqs.deps;
			it = find(deps.begin(), deps.end(), task_evaluation(name));
			assert (it != deps.end());
			it->failed_conds = failed;
			
			if (! ctx->seqs.all_deps_evaluated())
				return;

			std::sort(deps.begin(), deps.end(), class_task_evaluation());

			const conditionV& needed_precond = *deps[0].failed_conds;

			if (needed_precond.empty()) {
				a_.logger(INFORMATION) << ctx->ctr << " Executing " << deps[0].name;
				a_.logger(INFORMATION) << " to handle it " << std::endl;
				// XXX do it
			} else {
				a_.logger(INFORMATION) << ctx->ctr;
				a_.logger(INFORMATION) << " Need to handle the following conditions\n";
				std::copy(needed_precond.begin(), needed_precond.end(), 
						std::ostream_iterator<std::string>(a_.logger(3), "\n"));
				a_.logger(INFORMATION) << std::endl;
				// XXX do it
			}

		}

		logic_layer::~logic_layer() 
		{
		}
	}
}

