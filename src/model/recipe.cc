#include <network/log_level.hh>

#include <model/recipe.hh>
#include <model/ability.hh>
#include <model/task.hh>


using namespace hyper::model;
namespace {
	struct callback {
		ability & a;
		boost::optional<hyper::network::runtime_failure> res;

		callback(ability& a, boost::optional<hyper::network::runtime_failure> res) :
			a(a), res(res) {}	

		void operator()(recipe_execution_callback cb) { 
			a.io_s.post(boost::bind(cb, res));
		}
	};
}

recipe::recipe(const std::string& name, ability& a, task& t, 
			   boost::optional<logic::expression> error):
	name(name), a(a), t(t), expected_error_(error), is_running(false),
	must_pause(false), must_interrupt(false), computation(0), end_handler(0), prefer_(0) 
{}

void recipe::handle_end(const boost::system::error_code& e, const boost::system::error_code& e_exec) 
{
	a.logger(DEBUG) << "[Recipe " << name << "] End end handler execution " << e << std::endl;
	end_execute(!e_exec);
}

void recipe::handle_execute(const boost::system::error_code& e)
{
	a.logger(DEBUG) << "[Recipe " << name <<"] End real execution " << e << std::endl;

	if (has_end_handler) {
		a.logger(DEBUG) << "[Recipe " << name << "] Start end handler execution " << e << std::endl;
		do_end(boost::bind(&recipe::handle_end, this, _1, e));
	} else {
		end_execute(!e);
	}
}

void recipe::end_execute(bool res_)
{
	is_running = false;
	must_pause = false;

	boost::optional<hyper::network::runtime_failure> res;
	if (!res_ && !must_interrupt) 
		res = computation->error();
	if (computation != 0) {
		delete computation;
		computation = 0;
	}
	if (end_handler != 0) {
		delete end_handler;
		end_handler = 0;
	}

	std::for_each(pending_cb.begin(), pending_cb.end(),
				  callback(a, res));

	pending_cb.clear();
}

void recipe::execute(recipe_execution_callback cb)
{
	pending_cb.push_back(cb);
	if (is_running)
		return;

	is_running = true;
	must_interrupt = false;

	a.logger(DEBUG) << "[Recipe " << name <<"] Start real execution " << std::endl;
	do_execute(boost::bind(&recipe::handle_execute, this, _1), must_pause);
}

void recipe::abort()
{
	must_interrupt = true;
	if (is_running && computation)
		computation->abort();
}

void recipe::pause()
{
	must_pause = true;
	if (is_running && computation) {
		a.logger(DEBUG) << "[Recipe " << name << "] Pausing " << std::endl;
		computation->pause();
	}
}

void recipe::resume()
{
	must_pause = false;
	if (is_running && computation) {
		a.logger(DEBUG) << "[Recipe " << name << "] Resuming " << std::endl;
		computation->resume();
	}
}
