#include <model/ability.hh>
#include <model/compute_ensure_expression.hh>

namespace hyper {
	namespace model {
		compute_ensure_expression::compute_ensure_expression(
			ability& a, const std::string& dst, const std::string& ctr, 
			network::identifier& res) : 
			a(a), dst(dst), constraint(ctr), res_id(res), id(boost::none) {}

		void compute_ensure_expression::handle_end_computation(
				const boost::system::error_code& e,
				cb_type cb)
		{
			// in error case, go up to the caller, otherwise, let the work continue
			if (e) {
				a.db.remove(*id);
				cb(e);
			}
		}

		void compute_ensure_expression::compute(cb_type cb) 
		{
			id = boost::none;
			rqst.constraint =  constraint;
			rqst.repeat = true;

			id = a.client_db[dst].async_request(rqst, ans, 
					boost::bind(&compute_ensure_expression::handle_end_computation,
								this, boost::asio::placeholders::error, cb));

			res_id = *id;
			cb(boost::system::error_code());
		}

		void compute_ensure_expression::handle_abort(const boost::system::error_code&)
		{}

		void compute_ensure_expression::abort() 
		{
			if (!id)
				return;

			abort_msg.id = *id;
			a.client_db[dst].async_write(abort_msg, 
						boost::bind(&compute_ensure_expression::handle_abort,
									 this, boost::asio::placeholders::error));

		}
	}
}
