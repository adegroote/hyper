#ifndef HYPER_MODEL_COMPUTE_WHILE_EXPRESSION_HH_
#define HYPER_MODEL_COMPUTE_WHILE_EXPRESSION_HH_

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <model/abortable_function.hh>
#include <model/types.hh>
#include <network/msg_constraint.hh>

namespace hyper {
	namespace model {
		struct ability;

		/**
		 * A class which encapsulte a while computation 
		 */
		class compute_while_expression : public abortable_function_base {
				ability& a; /**< main agent reference */
				boost::shared_ptr<abortable_function_base> fun_cond_ptr;  /**< 
						encapsulation of condition predicate */
				boost::shared_ptr<abortable_computation> fun_body_ptr; /**<
						encapsulation of the body of the while */
				bool& res_condition; /**< result of the condition computation */

				bool running; /**< true if we are started a computation through
								compute() and it is still in progress */
				bool executing; /**< true if we are in the execution of the body */
				bool testing; /**< true if we are in the execution of the condition */

				bool must_interrupt; /**< user requests abort through abort() */
				bool must_pause; /**< user requests pause through pause() */
				cb_type cb_; /**< the callback to return at the end of the computation */

				/**
				 * Handle the terminaison of the condition
				 *
				 * @param e is the return code of the condition 
				 * @param cb is the final callback
				 */
				void handle_test(const boost::system::error_code& e, cb_type cb);

				/**
				 * Handle the terminaison of the body execution
				 *
				 * @param e is the return code of the body execution
				 * @param cb is the final callback
				 */
				void handle_computation(const boost::system::error_code& e, cb_type cb);

				/**
				 * Generic code to handle error cause, in function of the state (#must_interrupt, #must_pause).
				 *
				 * @param e is the return code to deal with
				 * @param cb is the final callback
				 * @return true if an error has been processed (and so the caller must stop any processing)
				 * @return false otherwise (in the nominal case)
				 */
				bool handle_error(const boost::system::error_code& e, cb_type cb);

			public:
				/**
				 * Constructor
				 *
				 * @param a is a reference to the main agent
				 * @param cond is a pointer to the condition predicate,
				 * compute_while_expression takes care about the lifetime of it
				 * @param res points to the result of the predicate condition
				 * @param fun is a pointer to an abortable computation, which
				 * represents body of while. compute_while_expression takes
				 * care about the lifetime of it
				 */
				compute_while_expression(ability& a, abortable_function_base* cond, bool &res, abortable_computation* fun):
					a(a), fun_cond_ptr(cond),  fun_body_ptr(fun), res_condition(res),
					running(false), testing(false), must_interrupt(false), must_pause(false)
				{}

				/**
				 * start the computation. it ends calling cb
				 *
				 * @param cb is the final callback of the method
				 */
				void compute (cb_type cb);
				
				/**
				 * Retrieve the error associated to this computation. It is
				 * valid only the last computation fails.
				 */
				logic::expression error() const { return fun_body_ptr->error(); }


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

#endif /* HYPER_MODEL_COMPUTE_WHILE_EXPRESSION_HH_ */
