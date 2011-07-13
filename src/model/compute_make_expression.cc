#include <model/ability.hh>
#include <model/compute_make_expression.hh>

namespace hyper {
	namespace model {
		compute_make_expression::compute_make_expression(
			ability& a, const std::string& dst, const std::string& ctr, bool& res) : 
			a(a), dst(dst), constraint(ctr), res(res), id(boost::none) {}

		void compute_make_expression::handle_end_computation(
				const boost::system::error_code& e,
				network::identifier id,
				cb_type cb)
		{
			a.db.remove(id);
			this->id = boost::none;

			if (e) 
				return cb(e);

			switch (ans.state) {
				case network::request_constraint_answer::INTERRUPTED:
					res = false;
					cb(make_error_code(boost::system::errc::interrupted));
					break;
				case network::request_constraint_answer::FAILURE:
					res = false;
					cb(make_error_code(exec_layer_error::execution_ko));
					break;
				case network::request_constraint_answer::SUCCESS:
					res = true;
					cb(boost::system::error_code());
					break;
			};
		}

		void compute_make_expression::compute(cb_type cb) 
		{
			id = boost::none;
			rqst.constraint =  constraint;
			rqst.repeat = false;

			id = a.client_db[dst].async_request(rqst, ans, 
					boost::bind(&compute_make_expression::handle_end_computation,
								this, boost::asio::placeholders::error, _2, cb));
		}

		void compute_make_expression::handle_abort(const boost::system::error_code&)
		{}

		bool compute_make_expression::abort() 
		{
			if (!id)
				return false;

			abort_msg.id = *id;
			abort_msg.src = a.name;
			a.client_db[dst].async_write(abort_msg, 
						boost::bind(&compute_make_expression::handle_abort,
									 this, boost::asio::placeholders::error));
			return true;
		}
	}
}
