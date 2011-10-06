#ifndef HYPER_MODEL_COMPUTE_ENSURE_EXPRESSION_HH_
#define HYPER_MODEL_COMPUTE_ENSURE_EXPRESSION_HH_

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <model/abortable_function.hh>
#include <network/msg_constraint.hh>
#include <model/types.hh>

namespace hyper {
	namespace model {
		struct ability;

		class compute_ensure_expression : public abortable_function_base {
				ability& a;
				model::identifier& res_id;
				std::string dst;
				logic::function_call f;
				boost::optional<network::identifier> id;
				network::request_constraint2 rqst;
				network::request_constraint_answer ans;
				network::abort abort_msg;

				void handle_end_computation(const boost::system::error_code& e,  cb_type);
				void handle_abort(const boost::system::error_code&);

			public:
				compute_ensure_expression(ability&, const std::string&, const logic::function_call& f,
										 const network::request_constraint2::unification_list&,
										 model::identifier &);

				void compute (cb_type cb);
				logic::expression error() const { return f; }
				bool abort();
		};
	}
}

#endif /* HYPER_MODEL_COMPUTE_ENSURE_EXPRESSION_HH_ */
