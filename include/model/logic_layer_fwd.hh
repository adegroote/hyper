#ifndef HYPER_MODEL_LOGIC_LAYER_FWD_HH_
#define HYPER_MODEL_LOGIC_LAYER_FWD_HH_

#include <boost/function/function1.hpp>
#include <boost/shared_ptr.hpp>

#include <network/msg_constraint.hh>

/** @file contains some definitions related to logic_layer, but used by others
 * unit-translation */

namespace hyper {
	namespace model {
		class ability;
		struct logic_context;
		typedef boost::shared_ptr<logic_context> logic_ctx_ptr;

		struct logic_layer;

		typedef boost::function<void (
				network::request_constraint_answer::state_, 
				const network::error_context&)>
		logic_layer_cb;

		struct logic_constraint
		{
			size_t id; 
			std::string src;
			bool repeat;  /**< mark difference btw make and ensure */
			bool internal; /**< internal (value update) or external */
			double delay; /** < time between test, if repeat. Unused otherwise */

			network::request_constraint_answer::state_ s; /**< current state */
		};

		/**
		 * Send a network::request_constraint_answer message with the current
		 * status of the constraint
		 *
		 * @param a the agent context
		 * @param ctr the constraint subject
		 */
		void update_ctr_status(hyper::model::ability& a, hyper::model::logic_constraint ctr,
														 const hyper::network::error_context&);

		inline
		std::ostream& operator << (std::ostream& oss, const logic_constraint& c)
		{
			oss << "[" << c.src << ", " << c.id << "]";
			return oss;
		}
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_FWD_HH_ */
