#include <network/log_level.hh>

#include <model/recipe.hh>
#include <model/ability.hh>


using namespace hyper::model;
namespace {
	struct callback {
		ability & a;
		bool res;

		callback(ability& a, bool res) : a(a), res(res) {}	

		void operator()(recipe_execution_callback cb) { 
			a.io_s.post(boost::bind(cb, res));
		}
	};
}

recipe::recipe(const std::string& name, ability& a):
	name(name), a(a), computation(0), is_running(false) 
{}

void recipe::handle_execute(const boost::system::error_code& e)
{
	a.logger(DEBUG) << "[Recipe " << name <<"] End real execution " << e << std::endl;
	end_execute(!e);
}

void recipe::end_execute(bool res)
{
	is_running = false;
	if (computation != 0) {
		delete computation;
		computation = 0;
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
	do_execute(boost::bind(&recipe::handle_execute, this, _1));
}

void recipe::abort()
{
	must_interrupt = true;
	if (is_running && computation)
		computation->abort();
}
