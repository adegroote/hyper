#include <model/task.hh>
#include <model/evaluate_conditions.hh>
#include <boost/test/unit_test.hpp>

#include <network/nameserver.hh>
#include <boost/assign/list_of.hpp>
#include <boost/thread.hpp>

namespace {
struct do_nothing
{
	void operator () (const boost::system::error_code&) {}
};

struct agent {
	boost::asio::io_service io_s;
	int i, j;
	int tested;
};

void check_i(const agent& a, bool& res, 
			 hyper::model::evaluate_conditions<2, agent>& eval, 
			 size_t i)
{
	res = (a.i == 42);
	eval.handle_computation(i);
}

void check_j(const agent& a, bool& res, 
			 hyper::model::evaluate_conditions<2, agent>& eval, 
			 size_t i)
{
	res = (a.j == 42);
	eval.handle_computation(i);
}

struct async_test {
	hyper::model::evaluate_conditions<2, agent> eval;
	agent & a_;

	async_test(agent& a) :
		eval(a, 
			boost::assign::list_of<hyper::model::evaluate_conditions<2,agent>::condition>
			(check_i, "check_i")(check_j, "check_j")), a_(a)
	{}

	void handle_second_test(hyper::model::conditionV vec)
	{
		BOOST_CHECK(vec.size() == 1);
		BOOST_CHECK(vec[0] == "check_j");
		a_.tested++;
	}

	void handle_first_test(hyper::model::conditionV vec)
	{
		BOOST_CHECK(vec.size() == 0);
		a_.tested++;
		a_.j = 4242;
		eval.async_compute(boost::bind(&async_test::handle_second_test, this, _1));
	}

	void test()
	{
		eval.async_compute(boost::bind(&async_test::handle_first_test, this, _1));
	}
};
}

BOOST_AUTO_TEST_CASE ( model_task_test )
{
	// check compilation
	using namespace hyper::model;
	using namespace hyper::network;

	boost::asio::io_service io_s;
	name_server s("127.0.0.1", "4242", io_s, false);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_s));

	{
	boost::asio::io_service ios2;
	hyper::network::name_client nc(ios2, "127.0.0.1", "4242");
	
	expression<boost::mpl::vector<> > e_empty;
	expression<boost::mpl::vector<int, double> > e_not_empty(io_s,
			boost::assign::list_of<std::pair<std::string, std::string> >
						("test", "x")("test","y"),
						nc);

	(void) e_empty;
	(void) e_not_empty;
	}

	agent our_agent;
	our_agent.i = 42;
	our_agent.j = 42;
	our_agent.tested = 0;

	async_test tester(our_agent);
	tester.test();
	our_agent.io_s.run();

	/* Make sure we are not here without making our test */
	BOOST_CHECK(our_agent.tested == 2);

	s.stop();
	thr.join();
}
