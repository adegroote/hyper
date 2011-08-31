#ifndef HYPER_MODEL_COMPUTE_MAKE_EXPRESSION_HH_
#define HYPER_MODEL_COMPUTE_MAKE_EXPRESSION_HH_

#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <model/abortable_function.hh>
#include <model/types.hh>
#include <network/msg_constraint.hh>

namespace hyper {
	namespace model {
		struct ability;

		class compute_make_expression : public abortable_function_base {
				ability& a;
				bool & res;
				std::string dst;
				logic::function_call f;
				network::request_constraint::unification_list unify_list;
				boost::optional<network::identifier> id;
				network::request_constraint2 rqst;
				network::request_constraint_answer ans;
				network::abort abort_msg;

				void handle_end_computation(const boost::system::error_code& e,  network::identifier, cb_type);
				void handle_abort(const boost::system::error_code&);

			public:
				compute_make_expression(ability&, const std::string&, const logic::function_call& f,
										const network::request_constraint::unification_list&,
										bool&);

				void compute (cb_type cb);
				bool abort();
		};
	}
}

#endif /* HYPER_MODEL_COMPUTE_MAKE_EXPRESSION_HH_ */
