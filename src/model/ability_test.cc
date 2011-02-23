#include <model/ability_test.hh>

using namespace hyper::model;

void dont_care(const boost::system::error_code& e)
{}

void ability_test::handle_send_constraint(const boost::system::error_code& e,
		network::request_constraint* msg,
		network::request_constraint_answer* ans)
{
	if (e) {
		std::cerr << "Failed to make " << msg->constraint << std::endl;
	} else {
		if (ans->success == true) {
			std::cerr << "Succesfully enforcing " << msg->constraint << std::endl;
		} else {
			std::cerr << "Failed to enforce " << msg->constraint << std::endl;
		}
	}
	delete msg;
	delete ans;
}


void ability_test::send_constraint(const std::string& constraint)
{
	network::request_constraint* msg(new network::request_constraint());
	network::request_constraint_answer *answer(new network::request_constraint_answer());
	msg->constraint = constraint;

	client_db[target].async_request(*msg, *answer, 
			boost::bind(&ability_test::handle_send_constraint,
				this,
				boost::asio::placeholders::error,
				msg, answer));
}

void ability_test::abort(const std::string& msg)
{
	network::terminate term(msg);
	client_db[target].async_write(term, &dont_care);
}
