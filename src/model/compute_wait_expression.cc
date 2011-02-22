#include <model/compute_wait_expression.hh>

namespace hyper {
	namespace model {
		compute_wait_expression::compute_wait_expression(boost::asio::io_service& io_s,
				boost::posix_time::time_duration delay,
				abortable_function_base* fun_ptr,
				bool &res) :
			fun_ptr(fun_ptr), io_service_(io_s), delay_(delay), deadline_(io_s),
			res(res), user_ask_abort(false)
		{}

		bool compute_wait_expression::handle_error(const boost::system::error_code& e, cb_type cb)
		{
			if (e) {
				cb(e);
				return true;
			}

			if (user_ask_abort) {
				cb(make_error_code(boost::system::errc::interrupted));
				return true;
			}

			return false;
		}

		void compute_wait_expression::handle_timeout(
				const boost::system::error_code& e, cb_type cb)
		{
			if (!handle_error(e, cb))
				compute(cb);
		}

		void compute_wait_expression::handle_computation(
				const boost::system::error_code& e, cb_type cb)
		{
			if (handle_error(e, cb)) return;

			if (res) return cb(boost::system::error_code());

			deadline_.expires_from_now(delay_);
			deadline_.async_wait(boost::bind(&compute_wait_expression::handle_timeout, 
						this,
						boost::asio::placeholders::error,
						cb));
		}

		void compute_wait_expression::compute(cb_type cb)
		{
			fun_ptr->compute(boost::bind(&compute_wait_expression::handle_computation, 
										 this,
										 boost::asio::placeholders::error,
										 cb));
		}

		void compute_wait_expression::abort()
		{
			fun_ptr->abort();
			user_ask_abort = true;
		}
	}
}
