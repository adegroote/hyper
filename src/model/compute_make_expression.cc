#include <model/ability.hh>
#include <model/actor_impl.hh>
#include <model/compute_make_expression.hh>

namespace hyper {
	namespace model {
		compute_make_expression::compute_make_expression(
			ability& a, const std::string& dst, const logic::function_call& f, 
			const network::request_constraint2::unification_list& unify_list,
			bool& res) : 
			a(a), res(res), dst(dst), f(f),
			id(boost::none), running(false),
			must_interrupt(false), must_pause(false)
		{
			rqst.constraint =  f;
			rqst.repeat = false;
			rqst.unify_list = unify_list;
			rqst.delay = 0.0;
		}

		void compute_make_expression::end(cb_type cb, const boost::system::error_code& e)
		{
			a.actor->db.remove(*id);
			id = boost::none;
			running = false;

			return cb(e);
		}

		void compute_make_expression::handle_end_computation(
				const boost::system::error_code& e,
				cb_type cb)
		{
			if (e) 
				return end(cb, e);

			switch (ans.state) {
				case network::request_constraint_answer::INTERRUPTED:
					res = false;
					end(cb, make_error_code(boost::system::errc::interrupted));
					break;
				case network::request_constraint_answer::FAILURE:
					res = false;
					if (must_interrupt)
						end(cb, make_error_code(boost::system::errc::interrupted));
					else
						end(cb, make_error_code(exec_layer_error::execution_ko));
					break;
				case network::request_constraint_answer::SUCCESS:
					if (must_interrupt) {
						res = false;
						end(cb, make_error_code(boost::system::errc::interrupted));
					} else {
						res = true;
						end(cb, boost::system::error_code());
					}
					break;
				default:
					/* We don't care about other state for the moment */
					break;
			};
		}

		void compute_make_expression::compute(cb_type cb) 
		{
			cb_ = cb;
			running = true;

			if (must_pause)
				return;
			
			id = a.actor->client_db[dst].async_request(rqst, ans, 
					boost::bind(&compute_make_expression::handle_end_computation,
								this, boost::asio::placeholders::error, cb));
		}

		void compute_make_expression::handle_write(const boost::system::error_code&)
		{}

		bool compute_make_expression::abort() 
		{
			if (!running or !id) 
				return false;

			running = false;
			must_interrupt = true;

			abort_msg.id = *id;
			abort_msg.src = a.name;
			a.actor->client_db[dst].async_write(abort_msg, 
						boost::bind(&compute_make_expression::handle_write,
									 this, boost::asio::placeholders::error));
			return true;
		}

		void compute_make_expression::pause() 
		{
			must_pause = true;
			if (!id) 
				return;

			pause_msg.src = a.name;
			pause_msg.id = *id;

			a.actor->client_db[dst].async_write(pause_msg, 
						boost::bind(&compute_make_expression::handle_write,
									 this, boost::asio::placeholders::error));

		}

		void compute_make_expression::resume() 
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
						boost::bind(&compute_make_expression::handle_write,
									 this, boost::asio::placeholders::error));

		}
	}
}
