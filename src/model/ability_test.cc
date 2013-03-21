#include <model/ability_test.hh>
#include <model/actor_impl.hh>
#include <model/future.hh>

using namespace hyper::model;

void ability_test::handle_send_constraint(const boost::system::error_code& e,
		network::identifier id,
		future_value<bool> res,
		network::request_constraint* msg,
		network::request_constraint_answer* ans)
{
	actor->db.remove(id);

	if (e) {
		std::cerr << "Failed to make " << msg->constraint << std::endl;
	} else {
		if (ans->state == network::request_constraint_answer::SUCCESS) {
			std::cerr << "Succesfully enforcing " << msg->constraint << std::endl;
		} else {
			std::cerr << "Failed to enforce " << msg->constraint << std::endl;
		}
	}
	res.get_raw() = (!e && 
					 ans->state == network::request_constraint_answer::SUCCESS);
	res.signal_ready();

	delete msg;
	delete ans;

	stop();
}


future_value<bool> ability_test::send_constraint(const std::string& constraint, bool repeat)
{
	network::request_constraint* msg(new network::request_constraint());
	network::request_constraint_answer *answer(new network::request_constraint_answer());
	msg->constraint = constraint;
	msg->repeat = repeat;

	future_value<bool> res(constraint);

	actor->client_db[target].async_request(*msg, *answer, 
			boost::bind(&ability_test::handle_send_constraint,
				this,
				boost::asio::placeholders::error,
				_2, res, msg, answer));

	return res;
}

int usage(const std::string& name)
{
	std::string prog_name = "hyper_" + name + "_test";
	std::cerr << "Usage :\n";
	std::cerr << prog_name << " get <var_name>\n";
	std::cerr << prog_name << " make <request>\n";
	std::cerr << prog_name << " ensure <request>\n";
	return -1;
}

int ability_test::main(int argc, char ** argv)
{
	std::vector<std::string> arguments(argv + 1, argv + argc);
	boost::function<void ()> to_execute;

	if (argc != 3)
		return usage(target);

	if (arguments[0] != "get" &&
		arguments[0] != "make" &&
		arguments[0] != "ensure")
		return usage(target);

	if (arguments[0] == "get") {
		getter_map::const_iterator it;
		it = gmap.find(arguments[1]);
		if (it != gmap.end()) {
			to_execute = it->second;
		} else
			std::cerr << target << " does not have such variable: " << arguments[1] << std::endl;
	}

	if (arguments[0] == "make") {
		to_execute = boost::bind(&ability_test::send_constraint, this,
									boost::cref(arguments[1]), false);
	}

	if (arguments[0] == "ensure") {
		to_execute = boost::bind(&ability_test::send_constraint, this,
									boost::cref(arguments[1]), true);
	}

	register_name();
	to_execute();
	run();

	return 0;
}
