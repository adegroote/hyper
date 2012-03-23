#include <model/ability.hh>
#include <model/actor_impl.hh>
#include <model/compute_ensure_expression.hh>

namespace hyper {
	namespace model {
		compute_ensure_expression::compute_ensure_expression(
			ability& a, const std::string& dst, const logic::function_call& f, 
			const network::request_constraint2::unification_list& unify_list,
			model::identifier& res) : 
			a(a), res_id(res), dst(dst), f(f),
			id(boost::none), running(false), must_pause(false) 
		{
			rqst.constraint =  f;
			rqst.repeat = true;
			rqst.unify_list = unify_list;
		}

		void compute_ensure_expression::end(cb_type cb, const boost::system::error_code& e)
		{
			a.actor->db.remove(*id);
			id = boost::none;
			running = false;
			if (e)
				return cb(e);
		}


		void compute_ensure_expression::handle_end_computation(
				const boost::system::error_code& e,
				cb_type cb)
		{
			if (e) 
				return end(cb, e);

			switch (ans.state) {
				case network::request_constraint_answer::SUCCESS:
					return;
				case network::request_constraint_answer::RUNNING:
					switch (state) {
						case network::request_constraint_answer::INIT:
							state = ans.state;
							return cb(boost::system::error_code());
						case network::request_constraint_answer::TEMP_FAILURE:
							state = ans.state;
							return cb(make_error_code(exec_layer_error::run_again));
						default:
							return;
					}
				case network::request_constraint_answer::TEMP_FAILURE:
					state = ans.state;
					return cb(make_error_code(exec_layer_error::temporary_failure));
				case network::request_constraint_answer::FAILURE:
					return end(cb, make_error_code(exec_layer_error::execution_ko));
				case network::request_constraint_answer::INTERRUPTED:
					return end(cb, make_error_code(boost::system::errc::interrupted));
				default:
					assert(false);
			}
		}

		void compute_ensure_expression::compute(cb_type cb) 
		{
			state = network::request_constraint_answer::INIT;
			running = true;
			cb_ = cb;
			
			if (must_pause) 
				return;

			id = a.actor->client_db[dst].async_request(rqst, ans, 
					boost::bind(&compute_ensure_expression::handle_end_computation,
								this, boost::asio::placeholders::error, cb));

			res_id.first = dst;
			res_id.second = *id;
		}

		void compute_ensure_expression::handle_write(const boost::system::error_code&)
		{}

		bool compute_ensure_expression::abort() 
		{
			running = false;

			if (!id) 
				return false;

			abort_msg.src = a.name;
			abort_msg.id = *id;

			a.actor->client_db[dst].async_write(abort_msg, 
						boost::bind(&compute_ensure_expression::handle_write,
									 this, boost::asio::placeholders::error));
			return true;

		}

		void compute_ensure_expression::pause() 
		{
			must_pause = true;

			if (!id) 
				return;

			pause_msg.src = a.name;
			pause_msg.id = *id;

			a.actor->client_db[dst].async_write(pause_msg, 
						boost::bind(&compute_ensure_expression::handle_write,
									 this, boost::asio::placeholders::error));

		}

		void compute_ensure_expression::resume() 
		{
			must_pause = false;

			if (!id) {
				if (running) {
					return compute(cb_);
				} else {
					return;
				}
			}

			resume_msg.src = a.name;
			resume_msg.id = *id;

			a.actor->client_db[dst].async_write(resume_msg, 
						boost::bind(&compute_ensure_expression::handle_write,
									 this, boost::asio::placeholders::error));

		}
	}
}
