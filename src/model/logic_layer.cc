#include <logic/expression.hh>
#include <logic/eval.hh>

#include <compiler/utils.hh>

#include <network/algorithm.hh>

#include <model/ability.hh>
#include <model/actor_impl.hh>
#include <model/compute_task_tree.hh>
#include <model/execute_impl.hh>
#include <model/setter_impl.hh>
#include <model/logic_layer_impl.hh>

namespace {
	using namespace hyper;

	struct transfrom_unification_error {};

	struct transform_unification {
		std::string name;

		transform_unification(const std::string& name) : name(name) {}

		model::unification_expr make_pair(const logic::expression& first, 
										  const logic::expression& second)
		{
			const std::string* s1 = boost::get<std::string>(& first.expr);
			const std::string* s2 = boost::get<std::string>(& second.expr);

			bool b1 = (s1 && compiler::scope::get_scope(*s1) == name);

			/* by construction b1 ^ b2 == true, see src/compiler/recipe.cc */
			if (b1) 
				return std::make_pair(*s1, second);
			else
				return std::make_pair(*s2, first);
		}

		model::unification_expr operator() (const model::unification_pair& p) {
			boost::optional<logic::expression> first, second;

			first = logic::generate_node(p.first);
			second = logic::generate_node(p.second);

			/* Must not happens ... */
			if (!first || !second)
				throw transfrom_unification_error();

			return make_pair(*first, *second);

		}

		model::unification_expr operator() (const model::unification_pair2& p) {
			return make_pair(p.first, p.second);
		}
	};

	template <typename T>
	void prepare_unification(const std::string& name, const T& unify, model::logic_ctx_ptr ctx) {
		ctx->unify_list.clear();
		std::transform(unify.begin(), unify.end(),
					   std::back_inserter(ctx->unify_list),
					   transform_unification(name));
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

		logic_context::logic_context(logic_layer& logic) : 
			logic_tree(logic, *this),
			deadline_(logic.a_.io_s)
		{}

		logic_context::logic_context(const logic_constraint& ctr_, logic_layer& logic) : 
				ctr(ctr_), logic_tree(logic, *this),
				deadline_(logic.a_.io_s)
		{}

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

		logic_ctx_ptr logic_layer::prepare_async_exec(const logic_constraint& ctr, logic_layer_cb cb)
		{
			logic_ctx_ptr ctx = boost::shared_ptr<logic_context>(
					new logic_context(ctr, *this));
			ctx->cb = cb;
			ctx->must_interrupt = false;
			ctx->must_pause = false;
			ctx->s_ = logic_context::IDLE;

			running_ctx[make_key(ctx)] = ctx;
			return ctx;
		}

		void logic_layer::async_exec_(logic_ctx_ptr ctx)
		{
			a_.logger(DEBUG) << ctx->ctr << " Try to set unification pattern " << std::endl;
			hyper::network::async_parallel_for_each(ctx->unify_list.begin(), ctx->unify_list.end(),
												   boost::bind(&setter::set, a_.setter, _1, _2), 
												   boost::bind(&logic_layer::handle_unification_computation,
															  this, boost::asio::placeholders::error, ctx));
		}

#define CHECK_INTERRUPT if (ctx->must_interrupt) \
							return handle_failure(ctx, make_error_code(boost::system::errc::interrupted));

		void logic_layer::async_exec(const logic_constraint& ctr, const std::string& constraint, 
									const unify_pair_list& list, logic_layer_cb cb)
		{
			logic_ctx_ptr ctx = prepare_async_exec(ctr, cb);

			logic::generate_return ret_exec =
						logic::generate(constraint, engine.funcs());

			if (ret_exec.res == false) {
				a_.logger(WARNING) << ctx->ctr << " Fail to parse " << constraint << std::endl;
				return handle_failure(ctx, make_error_code(logic_layer_error::parse_error));
			}
			ctx->call_exec = ret_exec.e;

			try {
				prepare_unification(a_.name, list, ctx);
			} catch (transfrom_unification_error) {
				return handle_failure(ctx, make_error_code(logic_layer_error::parse_error));
			}

			return async_exec_(ctx);
		}

		void logic_layer::async_exec(const logic_constraint& ctr, const logic::function_call& f,
									 const unify_pair_list2& list, logic_layer_cb cb)
		{
			logic_ctx_ptr ctx = prepare_async_exec(ctr, cb);

			logic::generate_return ret_exec =
						logic::generate(f, engine.funcs());

			if (ret_exec.res == false) {
				a_.logger(WARNING) << ctx->ctr << " Fail to adapt " << f << " to local context" << std::endl;
				return handle_failure(ctx, make_error_code(logic_layer_error::parse_error));
			}
			ctx->call_exec = ret_exec.e;
			prepare_unification(a_.name, list, ctx); // can't throw

			return async_exec_(ctx);
		}

		void logic_layer::handle_unification_computation(const boost::system::error_code& e, logic_ctx_ptr ctx)
		{
			CHECK_INTERRUPT

			if (e) 
				return handle_failure(ctx, e);

			ctx->s_ = logic_context::EXEC;

			a_.logger(DEBUG) << ctx->ctr << " Computation of the state " << std::endl;
			return async_eval_expression(a_.io_s, ctx->call_exec,
										  a_, ctx->exec_res,
				boost::bind(&logic_layer::handle_exec_computation, this,
							boost::asio::placeholders::error,
							ctx));
		}

		void logic_layer::handle_exec_task_tree(bool success, logic_ctx_ptr ctx)
		{
			CHECK_INTERRUPT

			a_.logger(DEBUG) <<  ctx->ctr << " End execution with ";
			a_.logger(DEBUG) << (success ? "success" : "failure")  << std::endl;
			if (success) 
				handle_success(ctx);
			else 
				if (ctx->ctr.internal)
					handle_failure(ctx, make_error_code(logic_layer_error::recipe_execution_error));
				else
					ctx->logic_tree.async_eval_cond(ctx->call_exec,
							boost::bind(&logic_layer::handle_eval_task_tree, this,
									   _1, ctx));
		}

		void logic_layer::handle_eval_task_tree(bool success, logic_ctx_ptr ctx) 
		{
			CHECK_INTERRUPT

			a_.logger(DEBUG) <<  ctx->ctr << " Finish computation of async_task " << std::endl;
			if (success) {
				a_.logger(DEBUG) <<  ctx->ctr << " Start execution " << std::endl;
				ctx->s_ = logic_context::LOGIC_EXEC;
				ctx->logic_tree.async_execute(
						boost::bind(&logic_layer::handle_exec_task_tree, this, _1, ctx));
			} else {
				a_.logger(DEBUG) << ctx->ctr << " No solution found ! " << std::endl;
				handle_failure(ctx, make_error_code(logic_layer_error::no_solution_found));
			}
		}

		void logic_layer::async_exec(const std::string& task, network::identifier id,
									 const std::string& src, logic_layer_cb cb)

		{
			logic_ctx_ptr ctx = boost::shared_ptr<logic_context>(
					new logic_context(*this));
			ctx->ctr.id = id;
			ctx->ctr.src = src;
			ctx->ctr.internal = true;
			ctx->ctr.repeat = false;
			ctx->cb = cb;
			ctx->must_interrupt = false;
			ctx->must_pause = false;
			ctx->s_ = logic_context::LOGIC;

			running_ctx[make_key(src, id)] = ctx;

			ctx->logic_tree.async_eval_task(task, 
					boost::bind(&logic_layer::handle_eval_task_tree, this,
							   _1, ctx));
		}


		void logic_layer::handle_exec_computation(const boost::system::error_code& e,
									 logic_ctx_ptr ctx)
		{
			CHECK_INTERRUPT

			a_.logger(DEBUG) <<  ctx->ctr << " Finish the constraint computation " << std::endl;

			if (e  || !ctx->exec_res) {
				a_.logger(DEBUG) << ctx->ctr << " Failed to evaluate" << std::endl;
				return handle_failure(ctx, make_error_code(logic_layer_error::evaluation_error));
			}

			if (ctx->exec_res && *(ctx->exec_res)) {
				a_.logger(INFORMATION) << ctx->ctr << " Already enforced" << std::endl;
				return handle_success(ctx);
			}

			ctx->s_ = logic_context::LOGIC_EXEC;

			ctx->logic_tree.async_eval_cond(ctx->call_exec,
					boost::bind(&logic_layer::handle_eval_task_tree, this,
							   _1, ctx));
		}

		void logic_layer::handle_success(logic_ctx_ptr ctx) 
		{
			CHECK_INTERRUPT

			if (ctx->ctr.repeat) {
				ctx->s_ = logic_context::WAIT;
				ctx->ctr.s = hyper::network::request_constraint_answer::RUNNING;
				update_ctr_status(a_, ctx->ctr);

				if (! ctx->must_pause) {
					ctx->deadline_.expires_from_now(boost::posix_time::milliseconds(ctx->ctr.delay));
					a_.logger(DEBUG) << ctx->ctr << " Sleeping " << ctx->ctr.delay << " ms" ;
					a_.logger(DEBUG) << "before verifying again the ctr " << std::endl;
					ctx->deadline_.async_wait(boost::bind(&logic_layer::handle_timeout, this,
														  boost::asio::placeholders::error,
														  ctx));
				}
			}
			else {
				running_ctx.erase(make_key(ctx));
				return ctx->cb(network::request_constraint_answer::SUCCESS);
			}
		}

		void logic_layer::handle_failure(logic_ctx_ptr ctx, const boost::system::error_code& e)
		{
			running_ctx.erase(make_key(ctx));
			if (e == boost::system::errc::interrupted)
				return ctx->cb(network::request_constraint_answer::INTERRUPTED);
			else
				return ctx->cb(network::request_constraint_answer::FAILURE);
		}

		void logic_layer::handle_timeout(const boost::system::error_code& e, logic_ctx_ptr ctx)
		{
			if (ctx->must_pause) return;

			CHECK_INTERRUPT

			a_.logger(DEBUG) << ctx->ctr << " Computation of the state " << std::endl;
			return async_eval_expression(a_.io_s, ctx->call_exec,
										  a_, ctx->exec_res,
				boost::bind(&logic_layer::handle_exec_computation, this,
							boost::asio::placeholders::error,
							ctx));
		}

		std::string logic_layer::make_key(const std::string& src, network::identifier id) const
		{
			std::ostringstream oss; 
			oss << src << id;
			return oss.str();
		}

		std::string logic_layer::make_key(logic_ctx_ptr ctx) const
		{
			return make_key(ctx->ctr.src, ctx->ctr.id);
		}

		void logic_layer::abort(const std::string& src, network::identifier id)
		{
			std::map<std::string, logic_ctx_ptr>::iterator it;
			it = running_ctx.find(make_key(src, id));
			if (it == running_ctx.end()) {
				a_.logger(DEBUG) << "Don't find ctx for request [" << src << ", " << id << "]";
				a_.logger(DEBUG) << std::endl;
				return;
			}

			it->second->must_interrupt = true;
			switch (it->second->s_) {
				case logic_context::LOGIC:
				case logic_context::LOGIC_EXEC:
					it->second->logic_tree.abort();
					break;
				case logic_context::WAIT:
					if (it->second->must_pause)
						return handle_failure(it->second, \
								make_error_code(boost::system::errc::interrupted));
					break;
				default:
					break;
			}
		}

		void logic_layer::pause(const std::string& src, network::identifier id)
		{
			std::map<std::string, logic_ctx_ptr>::iterator it;
			it = running_ctx.find(make_key(src, id));
			if (it == running_ctx.end()) {
				a_.logger(DEBUG) << "Don't find ctx for request [" << src << ", " << id << "]";
				a_.logger(DEBUG) << std::endl;
				return;
			}

			it->second->must_pause = true;
			switch (it->second->s_) {
				case logic_context::EXEC:
				case logic_context::LOGIC:
				case logic_context::LOGIC_EXEC:
					a_.logger(DEBUG) << "Will pause logic_tree " << std::endl;
					it->second->logic_tree.pause();
					break;
				case logic_context::WAIT:
					it->second->deadline_.cancel();
					break;
				default:
					break;
			}
		}

		void logic_layer::resume(const std::string& src, network::identifier id)
		{
			std::map<std::string, logic_ctx_ptr>::iterator it;
			it = running_ctx.find(make_key(src, id));
			if (it == running_ctx.end()) {
				a_.logger(DEBUG) << "Don't find ctx for request [" << src << ", " << id << "]";
				a_.logger(DEBUG) << std::endl;
				return;
			}

			it->second->must_pause = false;
			switch (it->second->s_) {
				case logic_context::EXEC:
				case logic_context::LOGIC:
				case logic_context::LOGIC_EXEC:
					it->second->logic_tree.resume();
					break;
				case logic_context::WAIT:
					return handle_timeout(boost::system::error_code(), it->second);
					break;
				default:
					break;
			}
		}

		logic::function_call logic_layer::generate(const logic::function_call& f) {
			logic::generate_return ret = logic::generate(f, engine.funcs());
			assert(ret.res);
			return ret.e;
		}

		logic_layer::~logic_layer() 
		{
		}
	}
}

