#ifndef HYPER_MODEL_COMPUTE_ABORT_EXPRESSION_HH_
#define HYPER_MODEL_COMPUTE_ABORT_EXPRESSION_HH_

#include <boost/optional/optional_fwd.hpp>
#include <boost/system/error_code.hpp>

#include <network/msg_constraint.hh>
#include <model/abortable_function.hh>
#include <model/types.hh>


namespace hyper {
	namespace model {
		struct ability;

		class compute_abort_expression : public abortable_function_base {
				ability& a;
				const model::identifier& id;
				network::abort abort_msg;

				void wait_terminaison(cb_type cb);
				void handle_abort(const boost::system::error_code&);

			public:
				compute_abort_expression(ability&, const model::identifier&, boost::mpl::void_&);

				void compute (cb_type cb);
				bool abort();
				void pause() {} 
				void resume() {}
		};
	}
}

#endif /* HYPER_MODEL_COMPUTE_ABORT_EXPRESSION_HH_ */
