#ifndef HYPER_MODEL_COMPUTE_ABORT_EXPRESSION_HH_
#define HYPER_MODEL_COMPUTE_ABORT_EXPRESSION_HH_

#include <boost/system/error_code.hpp>
#include <model/abortable_function.hh>
#include <model/types.hh>

#include <boost/optional/optional_fwd.hpp>

namespace hyper {
	namespace model {
		struct ability;

		class compute_abort_expression : public abortable_function_base {
				ability& a;
				const model::identifier& id;
				network::abort abort_msg;

				void handle_abort(const boost::system::error_code&);

			public:
				compute_abort_expression(ability&, const model::identifier&, boost::mpl::void_&);

				void compute (cb_type cb);
				bool abort();
		};
	}
}

#endif /* HYPER_MODEL_COMPUTE_ABORT_EXPRESSION_HH_ */
