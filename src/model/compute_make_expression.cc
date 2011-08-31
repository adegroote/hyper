#include <model/ability.hh>
#include <model/actor_impl.hh>
#include <model/compute_make_expression.hh>

namespace hyper {
	namespace model {
		compute_make_expression::compute_make_expression(
			ability& a, const std::string& dst, const logic::function_call& f, 
			const network::request_constraint::unification_list& unify_list,
			bool& res) : 
			a(a), dst(dst), f(f), unify_list(unify_list), 
			res(res), id(boost::none) 
		{
			rqst.constraint =  f;
			rqst.repeat = false;
			rqst.unify_list = unify_list;
		}

		void compute_make_expression::handle_end_computation(
				const boost::system::error_code& e,
				network::identifier id,
				cb_type cb)
		{
			a.actor->db.remove(id);
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
			id = a.actor->client_db[dst].async_request(rqst, ans, 
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
			a.actor->client_db[dst].async_write(abort_msg, 
						boost::bind(&compute_make_expression::handle_abort,
									 this, boost::asio::placeholders::error));
			return true;
		}
	}
}
