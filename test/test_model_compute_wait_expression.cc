#include <iostream>

#include <model/compute_wait_expression.hh>

#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>

#include <boost/test/unit_test.hpp>

namespace {
	struct test_function : public hyper::model::abortable_function_base {
		bool res_;
		size_t counter;

		test_function() : res_(false), counter(0) {}

		void compute(cb_type cb)
		{
			std::cout << "counter " << counter << std::endl;
			++counter;
			res_ = (counter == 42);
			cb(boost::system::error_code());
		}

		bool abort() { return false; }

		void pause() {}
		void resume() {}

		void reset() { res_ = false; counter = 0; }

		bool& res() { return res_;}
	};

	struct test_compute_wait_expression {
		test_function *fun_ptr;
		hyper::model::abortable_computation* ptr; // owned by wait
		hyper::model::compute_wait_expression wait;
		boost::asio::deadline_timer deadline;
		int handled_test;

		test_compute_wait_expression(boost::asio::io_service& io_s):
			fun_ptr(new test_function()), 
			ptr(new hyper::model::abortable_computation()),
			wait(io_s, boost::posix_time::milliseconds(20), ptr, fun_ptr->res()),
			deadline(io_s)
		{
			ptr->push_back(fun_ptr);
		}

		void handle_timeout(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			wait.abort();
		}

		void handle_second_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(e);
			BOOST_CHECK(e == boost::system::errc::interrupted);
			BOOST_CHECK(fun_ptr->res() == false);

			handled_test++;
		}

		void handle_first_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(fun_ptr->res() == true);
			BOOST_CHECK(fun_ptr->counter == 42);

			handled_test++;

			fun_ptr->reset();
			/* Start the computing, but abort before its end */
			wait.compute(boost::bind(&test_compute_wait_expression::handle_second_test,
									 this, _1));
			deadline.expires_from_now(boost::posix_time::milliseconds(100));
			deadline.async_wait(boost::bind(&test_compute_wait_expression::handle_timeout, 
						this,
						boost::asio::placeholders::error));
		}

		void async_test() 
		{
			wait.compute(boost::bind(&test_compute_wait_expression::handle_first_test,
									 this, _1));
		}
	};
}

BOOST_AUTO_TEST_CASE ( model_compute_wait_expression_test )
{
	boost::asio::io_service io_s;
	test_compute_wait_expression test(io_s);
	test.handled_test = 0;
	test.async_test();
	io_s.run();

	BOOST_CHECK(test.handled_test == 2);
}
