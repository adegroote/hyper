#include <logic/expression.hh>
#include <logic/eval.hh>

#include <model/ability.hh>
#include <model/compute_task_tree.hh>
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
		const char* logic_layer_category_impl::name() const {
			return "logic_layer";
		}

		std::string logic_layer_category_impl::message(int ev) const
		{
			switch(ev) {
				case logic_layer_error::ok :
					return "ok";
				case logic_layer_error::parse_error :
					return "parse_error";
				case logic_layer_error::evaluation_error :
					return "evaluation_error";
				case logic_layer_error::recipe_execution_error :
					return "recipe_execution_error";
				default:
					return "unknow_error";
			}
		}

		const boost::system::error_category& logic_layer_category()
		{
			static logic_layer_category_impl instance;
			return instance;
		}

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

		void logic_layer::async_exec(const logic_constraint& ctr, logic_layer_cb cb)
									 
		{
			logic_ctx_ptr ctx = boost::shared_ptr<logic_context>(
					new logic_context(ctr, *this));
			ctx->cb = cb;

			std::string to_execute = prepare_execution_rqst(ctx->ctr.constraint);
			logic::generate_return ret_exec =
						logic::generate(to_execute, execFuncs);

			if (ret_exec.res == false) {
				a_.logger(WARNING) << ctr << " Fail to parse" << std::endl;
				return ctx->cb(make_error_code(logic_layer_error::parse_error));
			}

			ctx->call_exec = ret_exec.e;

			a_.logger(DEBUG) << ctr << " Computation of the state " << std::endl;
			return async_eval_expression(a_.io_s, ctx->call_exec,
										  a_, ctx->exec_res,
				boost::bind(&logic_layer::handle_exec_computation, this,
							boost::asio::placeholders::error,
							ctx));
		}

		void logic_layer::handle_exec_task_tree(bool success, logic_ctx_ptr ctx)
		{
			a_.logger(DEBUG) <<  ctx->ctr << " End execution " << success << std::endl;
			if (success) 
				ctx->cb(boost::system::error_code());
			else 
				ctx->cb(make_error_code(logic_layer_error::recipe_execution_error));
		}

		void logic_layer::handle_eval_task_tree(bool success, logic_ctx_ptr ctx) 
		{
			a_.logger(DEBUG) <<  ctx->ctr << " Finish computation of async_task " << std::endl;
			if (success) {
				a_.logger(DEBUG) <<  ctx->ctr << " Start execution " << std::endl;
				ctx->logic_tree.async_execute(
						boost::bind(&logic_layer::handle_exec_task_tree, this, _1, ctx));
			} else {
				a_.logger(DEBUG) << ctx->ctr << " No solution found ! " << std::endl;
				ctx->cb(make_error_code(logic_layer_error::no_solution_found));
			}
		}

		void logic_layer::async_exec(const std::string& task, network::identifier id,
									 const std::string& src, logic_layer_cb cb)

		{
			logic_ctx_ptr ctx = boost::shared_ptr<logic_context>(
					new logic_context(*this));
			ctx->ctr.id = id;
			ctx->ctr.src = src;
			ctx->cb = cb;

			ctx->logic_tree.async_eval_task(task, 
					boost::bind(&logic_layer::handle_eval_task_tree, this,
							   _1, ctx));
		}

		void logic_layer::handle_exec_computation(const boost::system::error_code& e,
									 logic_ctx_ptr ctx)
		{
			a_.logger(DEBUG) <<  ctx->ctr << " Finish the constraint computation " << std::endl;

			if (e  || !ctx->exec_res) {
				a_.logger(DEBUG) << ctx->ctr << " Failed to evaluate" << std::endl;
				return ctx->cb(make_error_code(logic_layer_error::evaluation_error));
			}

			if (ctx->exec_res && *(ctx->exec_res)) {
				a_.logger(INFORMATION) << ctx->ctr << " Already enforced" << std::endl;
				return ctx->cb(boost::system::error_code());
			}

			std::string to_logic = prepare_logic_rqst(ctx->ctr.constraint);
			ctx->logic_tree.async_eval_cond(to_logic, 
					boost::bind(&logic_layer::handle_eval_task_tree, this,
							   _1, ctx));
		}


		logic_layer::~logic_layer() 
		{
		}
	}
}

