#include <logic/expression.hh>
#include <logic/eval.hh>

#include <compiler/utils.hh>

#include <model/ability.hh>
#include <model/compute_task_tree.hh>
#include <model/execute_impl.hh>
#include <model/logic_layer_impl.hh>

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
		}

		void logic_layer::add_logic_type(const std::string& type) {
			engine.add_type(type);
		}

		void logic_layer::async_exec(const logic_constraint& ctr, logic_layer_cb cb)
									 
		{
			logic_ctx_ptr ctx = boost::shared_ptr<logic_context>(
					new logic_context(ctr, *this));
			ctx->cb = cb;

			logic::generate_return ret_exec =
						logic::generate(ctx->ctr.constraint, engine.funcs());

			if (ret_exec.res == false) {
				a_.logger(WARNING) << ctr << " Fail to parse " << ctx->ctr.constraint << std::endl;
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

			ctx->logic_tree.async_eval_cond(ctx->ctr.constraint,
					boost::bind(&logic_layer::handle_eval_task_tree, this,
							   _1, ctx));
		}


		logic_layer::~logic_layer() 
		{
		}
	}
}

