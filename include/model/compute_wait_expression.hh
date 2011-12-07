#ifndef HYPER_WAIT_COMPUTE_WAIT_EXPRESSION_HH_
#define HYPER_WAIT_COMPUTE_WAIT_EXPRESSION_HH_

#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/scoped_ptr.hpp>

#include <model/abortable_function.hh>

namespace hyper {
	namespace model {
		/** compute_wait_expression represents a computation represented by
		 * #fun_ptry which waits until a certain predicate is true.
		 * 
		 * It is implemented as a loop test -> wait -> test -> wait ..., with a
		 * configurable wait #delay_.
		 */
		class compute_wait_expression : public abortable_function_base {
			private:
				boost::scoped_ptr<abortable_function_base> fun_ptr; /**< 
					function code to execute at each wait step */
				boost::asio::io_service& io_service_; /**< ref to io_service */
				boost::posix_time::time_duration delay_; /**< wait time between two call of fun_ptr */
				boost::asio::deadline_timer deadline_; /**< timer to wait #delay_ */
				bool& res; /**< result of the wait, terminate when true */
				bool user_ask_abort; /**< true if abort has been called */
				bool must_pause; /**< true if pause has been called, and not resumed */
				bool running; /**< true if compute has not terminated */
				bool waiting; /**< true if in the wait phase */
				cb_type cb_; /**< store the final callback */

				typedef boost::function<void (const boost::system::error_code&)> cb_type;

				/**
				 * Called when #deadline_ has expired
				 * @param e is the return code of #deadline_::async_wait
				 * @param cb is the final callback
				 */
				void handle_timeout(const boost::system::error_code& e, cb_type cb);

				/**
				 * Called when #fun_ptr->compute() has terminated()
				 * @param e is the return code of compute
				 * @param cb is the final callback
				 */
				void handle_computation(const boost::system::error_code& e, cb_type cb);

				/**
				 * Generic wait to handle the different error case. Deal with
				 * the different possible states (pause, abort, ...)
				 *
				 * @return true if we have deal with an error case (and in this
				 * case, the caller must stop any processing.
				 * @return false otherwise (in the nominal case)
				 */
				bool handle_error(const boost::system::error_code& e, cb_type cb);

			public:
				/**
				 * The constructor
				 *
				 * @param io_s is the io_service needed for boost::asio call
				 * @param delay is the time between two tests call
				 * @param fun_ptr represents the test function : it must update res when invocated
				 * @param res is the result of the test.
				 */
				compute_wait_expression(boost::asio::io_service& io_s,
						boost::posix_time::time_duration delay,
						abortable_function_base* fun_ptr,
						bool& res);

				/**
				 * Launch the loop compute - wait
				 * @param cb is the callback call when the action finishes
				 */
				void compute(cb_type cb);

				/**
				 * Abort the action.
				 */

				bool abort();

				/**
				 * Pause the current action
				 */
				void pause();

				/**
				 * Resume the current action
				 */
				void resume();
		};
	}
}

#endif /* HYPER_WAIT_COMPUTE_WAIT_EXPRESSION_HH_ */
