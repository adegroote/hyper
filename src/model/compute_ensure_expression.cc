#include <model/ability.hh>
#include <model/compute_ensure_expression.hh>

namespace hyper {
	namespace model {
		compute_ensure_expression::compute_ensure_expression(
			ability& a, const std::string& dst, const std::string& ctr, 
			model::identifier& res) : 
			a(a), dst(dst), constraint(ctr), res_id(res), id(boost::none) {}

		void compute_ensure_expression::handle_end_computation(
				const boost::system::error_code& e,
				cb_type cb)
		{
			// in error case, go up to the caller, otherwise, let the work continue
			if (e || ans.state != network::request_constraint_answer::SUCCESS) {
				a.db.remove(*id);
				id = boost::none;
				if (e)
					return cb(e);

				switch (ans.state) {
					case network::request_constraint_answer::FAILURE:
						return cb(make_error_code(exec_layer_error::execution_ko));
					case network::request_constraint_answer::INTERRUPTED:
						return cb(make_error_code(boost::system::errc::interrupted));
					default:
						assert(false);
				}
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

			res_id.first = dst;
			res_id.second = *id;
			cb(boost::system::error_code());
		}

		void compute_ensure_expression::handle_abort(const boost::system::error_code&)
		{}

		bool compute_ensure_expression::abort() 
		{
			if (!id) 
				return false;

			abort_msg.src = a.name;
			abort_msg.id = *id;
			a.client_db[dst].async_write(abort_msg, 
						boost::bind(&compute_ensure_expression::handle_abort,
									 this, boost::asio::placeholders::error));
			return true;

		}
	}
}
