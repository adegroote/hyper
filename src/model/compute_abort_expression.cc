#include <model/ability.hh>
#include <model/actor_impl.hh>
#include <model/compute_abort_expression.hh>

namespace hyper {
	namespace model {
		compute_abort_expression::compute_abort_expression(
			ability& a, const model::identifier& id,
			boost::mpl::void_& res) : 
			a(a), id(id) {}

		void compute_abort_expression::handle_abort(const boost::system::error_code&)
		{}

		void compute_abort_expression::compute(cb_type cb) 
		{
			abort_msg.src = a.name;
			abort_msg.id = id.second;
			a.actor->client_db[id.first].async_write(abort_msg, 
						boost::bind(&compute_abort_expression::handle_abort,
									 this, boost::asio::placeholders::error));
			cb(boost::system::error_code());
		}

		bool compute_abort_expression::abort() 
		{
			return false;
		}
	}
}
