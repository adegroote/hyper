#include <model/task.hh>
#include <boost/test/unit_test.hpp>

#include <network/nameserver.hh>
#include <boost/assign/list_of.hpp>
#include <boost/thread.hpp>

BOOST_AUTO_TEST_CASE ( model_task_test )
{
	// check compilation
	using namespace hyper::model;
	using namespace hyper::network;

	boost::asio::io_service io_s;
	name_server s("127.0.0.1", "4242", io_s, false);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_s));

	boost::asio::io_service ios2;
	hyper::network::name_client nc(ios2, "127.0.0.1", "4242");
	
	expression<boost::mpl::vector<> > e_empty;
	expression<boost::mpl::vector<int, double> > e_not_empty(io_s, 
			boost::assign::list_of<std::pair<std::string, std::string> >
						("test", "x")("test","y"),
						nc);

	(void) e_empty;
	(void) e_not_empty;

	s.stop();
	thr.join();
}
