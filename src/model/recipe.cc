
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
	name(name), a(a), is_running(false), thr(0)
{}

recipe::~recipe() { 
	if (thr) {
		thr->join(); 
		delete thr; 
	}
}

void recipe::handle_execute(bool res)
{
	thr->join();
	delete thr;
	thr = 0;

	end_execute(res);
}

void recipe::end_execute(bool res)
{
	is_running = false;

	std::for_each(pending_cb.begin(), pending_cb.end(),
				  callback(a, res));

	pending_cb.clear();
}

void recipe::real_execute()
{
	bool res = do_execute();
	a.io_s.post(boost::bind(&recipe::handle_execute, this, res));
}

void recipe::execute(recipe_execution_callback cb)
{
	pending_cb.push_back(cb);
	if (is_running)
		return;

	is_running = true;

	async_evaluate_preconditions(
			boost::bind(&recipe::handle_evaluate_preconditions, this, _1));
}

				

void recipe::handle_evaluate_preconditions(conditionV error)
{
	if (!error.empty())
		return end_execute(false);

	thr = new boost::thread(boost::bind(&recipe::real_execute, this));
}

