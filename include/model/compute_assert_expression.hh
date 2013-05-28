#ifndef HYPER_ASSERT_COMPUTE_ASSERT_EXPRESSION_HH_
#define HYPER_ASSERT_COMPUTE_ASSERT_EXPRESSION_HH_

#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/scoped_ptr.hpp>

#include <model/abortable_function.hh>
#include <model/types.hh>

namespace hyper {
	namespace model {
		/** compute_assert_expression represents a computation represented by
		 * #fun_ptry which verify that a certain predicate is true during the
		 * whole execution
		 * 
		 * It is implemented as a loop test -> wait -> test -> wait ..., with a
		 * configurable wait #delay_.
		 *
		 * The passed callback is called once with error::ok to signal that it
		 * is started to its caller. It is then called only in case of error,
		 * or interruption, with exec_layer_error::execution_ko in the first
		 * case, and system::interrupted in the second case. In the first case,
		 * failure is filled properly.
		 */
		class compute_assert_expression : public abortable_function_base {
			private:
				boost::scoped_ptr<abortable_computation> fun_ptr; /**< 
					function code to execute at each assert step */
				logic::function_call f;

				boost::asio::io_service& io_service_; /**< ref to io_service */
				boost::posix_time::time_duration delay_; /**< wait time between two call of fun_ptr */
				boost::asio::deadline_timer deadline_; /**< timer to wait #delay_ */
				model::identifier& res; /**< the identifier filled with the reference to this computation */
				bool& res_fun; /**< modified by a call to fun_ptr */

				bool user_ask_abort; /**< true if abort has been called */
				bool must_pause; /**< true if pause has been called, and not resumed */
				bool running; /**< true if compute has not terminated */
				bool waiting; /**< true if in the wait phase */
				cb_type cb_; /**< store the final callback */
				size_t idx_; /** < store the index of the assert, in the englobing computation */
				bool first_call; /** < check if it is the first call */
				hyper::network::runtime_failure failure; /** store the failure, if any */

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
				 * @param fun_ptr represents the test function
				 * @param f represents the computation done by fun_ptr, which
				 * will be returned by error
				 * @param res is the output of assert, it allows to furtherly stop it
				 * @param res_fun is the bool where is the stored the result of
				 * the computation fun_ptr
				 * @param idx reprensents the index of the assert in its owner
				 * computation
				 */
				compute_assert_expression(boost::asio::io_service& io_s,
						boost::posix_time::time_duration delay,
						abortable_computation* fun_ptr,
						const logic::function_call& f,
						model::identifier& res, bool& res_fun, size_t idx);

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

				network::runtime_failure error() const { return failure; }
		};
	}
}

#endif /* HYPER_ASSERT_COMPUTE_ASSERT_EXPRESSION_HH_ */
