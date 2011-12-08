#include <model/ability.hh>
#include <model/actor_impl.hh>
#include <model/compute_while_expression.hh>

namespace hyper {
	namespace model {

		bool compute_while_expression::handle_error(const boost::system::error_code& e, cb_type cb)
		{
			if (must_pause) {
				return true;
			}

			if (must_interrupt) {
				running = false;
				cb(make_error_code(boost::system::errc::interrupted));
				return true;
			}

			if (e) {
				running = false;
				cb(e);
				return true;
			}

			return false;
		}

		void compute_while_expression::handle_computation(const boost::system::error_code& e, cb_type cb)
		{
			executing = false;

			if (handle_error(e, cb)) return;

			compute(cb);
		}

		void compute_while_expression::handle_test(const boost::system::error_code& e, cb_type cb)
		{
			testing = false;

			if (handle_error(e, cb)) return;

			if (!res_condition) {
				running = false;
				return cb(boost::system::error_code());
			}


			executing = true;
			cb_type local_cb = boost::bind(&compute_while_expression::handle_computation, this, _1, cb);
			/* make sure the system has some time to call others handler */
			a.io_s.post(boost::bind(&abortable_computation::compute, fun_body_ptr, local_cb));
		}

		void compute_while_expression::compute(cb_type cb) 
		{
			cb_ = cb;
			running = true;
			testing = true;
			executing = false;

			cb_type local_cb = boost::bind(&compute_while_expression::handle_test, this, _1, cb);
			/* make sure the system has some time to call others handler */
			a.io_s.post(boost::bind(&abortable_function_base::compute, fun_cond_ptr, local_cb));
		}

		bool compute_while_expression::abort() 
		{
			must_interrupt = true;

			if (must_pause) {
				running = false;
				return false;
			}

			if (testing) {
				fun_cond_ptr->abort();
				return true;
			}

			if (executing) {
				fun_body_ptr->abort();
				return true;
			}

			return false;
		}

		void compute_while_expression::pause() 
		{
			must_pause = true;
			if (testing)
				fun_cond_ptr->pause();

			if (executing)
				fun_body_ptr->resume();
		}

		void compute_while_expression::resume() 
		{
			must_pause = false;
			if (testing)
				fun_cond_ptr->resume();

			if (executing) 
				fun_body_ptr->resume();

			if (!testing && !executing && running)
				compute(cb_);
		}
	}
}
