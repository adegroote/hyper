#include <network/log.hh>
#include <boost/test/unit_test.hpp>

#include <boost/thread/thread.hpp>

namespace {
	struct do_nothing
	{
		void operator () (const boost::system::error_code&) {}
	};

	typedef boost::mpl::vector<hyper::network::log_msg> input_msg;
	typedef boost::mpl::vector<boost::mpl::void_> output_msg;

	typedef boost::make_variant_over<input_msg>::type input_variant;
	typedef boost::make_variant_over<output_msg>::type output_variant;

	struct logger_visitor : public boost::static_visitor<output_variant>
	{
		std::vector<std::string> & log_msgs_;

		logger_visitor(std::vector<std::string>& log_msgs): log_msgs_(log_msgs) {}

		output_variant operator() (const hyper::network::log_msg& msg) const
		{
			log_msgs_.push_back(msg.msg);
			return boost::mpl::void_();
		}
	};

	struct false_resolv
	{
		false_resolv() {};

		template <typename Handler>
		void async_resolve(hyper::network::name_resolve & solv, Handler handler)
		{
			solv.rna.endpoint = boost::asio::ip::tcp::endpoint(
					  boost::asio::ip::address::from_string("127.0.0.1"), 4242);
			handler(boost::system::error_code());
		}
	};
	
}

BOOST_AUTO_TEST_CASE ( network_logger_test )
{
	typedef hyper::network::tcp::server<input_msg, output_msg, logger_visitor> logger_server;

	boost::asio::io_service io_s;
	std::vector<std::string> res;
	logger_visitor vis(res);
	logger_server serv("127.0.0.1", "4242", vis, io_s);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_s));

	boost::asio::io_service io_s2;
	false_resolv solver;

	boost::asio::deadline_timer deadline_(io_s2);
	deadline_.expires_from_now(boost::posix_time::milliseconds(10));
	deadline_.async_wait(do_nothing());
	hyper::network::async_logger<false_resolv> logger_(io_s2, "test", "logger", solver);
	logger_.log("first message");
	io_s2.run();
	io_s2.reset();

	deadline_.expires_from_now(boost::posix_time::milliseconds(10));
	deadline_.async_wait(do_nothing());
	logger_.log("second message");
	io_s2.run();
	io_s2.reset();

	std::vector<std::string> expected;
	expected.push_back("first message");
	expected.push_back("second message");
	BOOST_CHECK(res == expected);

	res.clear();

	hyper::network::logger<false_resolv> real_logger_(io_s2, "test", "logger", solver, 3);
	deadline_.expires_from_now(boost::posix_time::milliseconds(10));
	deadline_.async_wait(do_nothing());
	real_logger_(4) << "first message" << std::endl;
	io_s2.run();
	io_s2.reset();

	deadline_.expires_from_now(boost::posix_time::milliseconds(10));
	deadline_.async_wait(do_nothing());
	real_logger_(5) << "second message" << std::endl;
	io_s2.run();
	io_s2.reset();

	deadline_.expires_from_now(boost::posix_time::milliseconds(10));
	deadline_.async_wait(do_nothing());
	real_logger_(2) << "third message" << std::endl;
	io_s2.run();
	io_s2.reset();

	deadline_.expires_from_now(boost::posix_time::milliseconds(10));
	deadline_.async_wait(do_nothing());
	real_logger_(3) << "fourth message" << std::endl;
	io_s2.run();
	io_s2.reset();

	expected.push_back("fourth message");
	BOOST_CHECK(res == expected);

	serv.stop();
	thr.join();
}
