#include <model/ability.hh>
#include <model/actor_impl.hh>
#include <model/compute_abort_expression.hh>

namespace hyper {
	namespace model {
		compute_abort_expression::compute_abort_expression(
			ability& a, 
			abortable_computation& computation,
			const model::identifier& id,
			boost::mpl::void_& ) : 
			a(a), computation_(computation), id(id) {}

		void compute_abort_expression::wait_terminaison(cb_type cb) {
			cb(boost::system::error_code());
		}

		void compute_abort_expression::compute(cb_type cb) 
		{
			computation_.abort(id.idx, 
					boost::bind(&compute_abort_expression::wait_terminaison, this, cb));
		}

		bool compute_abort_expression::abort() 
		{
			return false;
		}
	}
}
