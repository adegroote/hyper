#include <model/compute_assert_expression.hh>

#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>

namespace hyper {
	namespace model {
		compute_assert_expression::compute_assert_expression(boost::asio::io_service& io_s,
				boost::posix_time::time_duration delay,
				abortable_computation* fun_ptr,
				const logic::function_call& f,
				model::identifier &res, bool& res_fun, size_t idx) :
			fun_ptr(fun_ptr), f(f), io_service_(io_s), delay_(delay), deadline_(io_s),
			res(res), res_fun(res_fun),
			user_ask_abort(false), must_pause(false), running(false), waiting(false), 
			idx_(idx), first_call(true)
		{}

		bool compute_assert_expression::handle_error(const boost::system::error_code& e, cb_type cb)
		{
			if (user_ask_abort) {
				cb(make_error_code(boost::system::errc::interrupted));
				running = false;
				return true;
			}

			if (e) {
				cb(e);
				running = false;
				return true;
			}

			return false;
		}

		void compute_assert_expression::handle_timeout(
				const boost::system::error_code& e, cb_type cb)
		{
			waiting = false;
			if (!handle_error(e, cb))
				compute(cb);
		}

		void compute_assert_expression::handle_computation(
				const boost::system::error_code& e, cb_type cb)
		{
			if (handle_error(e, cb)) return;

			if (!res_fun) { 
				running = false;
				return cb(make_error_code(exec_layer_error::execution_ko));
			}

			if (must_pause)
				return;

			waiting = true;
			deadline_.expires_from_now(delay_);
			deadline_.async_wait(boost::bind(&compute_assert_expression::handle_timeout, 
						this,
						boost::asio::placeholders::error,
						cb));
		}

		void compute_assert_expression::compute(cb_type cb)
		{
			running = true;
			cb_ = cb;
			if (must_pause)
				return;

			res.idx = idx_;

			fun_ptr->compute(boost::bind(&compute_assert_expression::handle_computation, 
										 this,
										 boost::asio::placeholders::error,
										 cb));

			if (first_call) {
				cb(boost::system::error_code());
				first_call = false;
			}
		}

		bool compute_assert_expression::abort()
		{
			if (!running)
				return false;

			user_ask_abort = true;

			if (must_pause) {
				running = false;
				return waiting;
			}

			if (!waiting) 
				fun_ptr->abort();

			return true;
		}

		void compute_assert_expression::pause()
		{
			must_pause = true;
		}

		void compute_assert_expression::resume()
		{
			must_pause = false;
			if (running) {
				compute(cb_);
			}
		}
	}
}
