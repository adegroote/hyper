#include <iostream>

#include <model/abortable_function.hh>

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>

namespace {
	void add(int& x, int& y, int& z, boost::function<void (const boost::system::error_code&)> cb)
	{
		z+= x + y;
		cb(boost::system::error_code());
	}

	void times(int& x, int& y, int& z, boost::function<void (const boost::system::error_code&)> cb)
	{
		z+= x * y;
		cb(boost::system::error_code());
	}

	void error_times(int& x, int& y, int& z, boost::function<void (const boost::system::error_code&)> cb)
	{
		z+= x * y;
		cb(boost::system::error_code(42, boost::system::generic_category()));
	}

	struct test_callback
	{
		int x, y, z;
		int handled_test;

		hyper::model::abortable_computation computation;

		void handle_second_test(const boost::system::error_code &e)
		{
			BOOST_CHECK(e);
			BOOST_CHECK(x == 12);
			BOOST_CHECK(y == 3);
			BOOST_CHECK(z == 66); // the computation has stopped after error_times

			handled_test++;
		}

		void handle_first_test(const boost::system::error_code &e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(x == 12);
			BOOST_CHECK(y == 3);
			BOOST_CHECK(z == 102);

			z = 0;
			computation.clear();
			computation.push_back(new hyper::model::abortable_function(boost::bind(&add, 
																	 boost::ref(x), boost::ref(y), 
																	 boost::ref(z), _1)));
			computation.push_back(new hyper::model::abortable_function(boost::bind(&add, 
																	 boost::ref(x), boost::ref(y), 
																	 boost::ref(z), _1)));
			computation.push_back(new hyper::model::abortable_function(boost::bind(&error_times, 
																	 boost::ref(x), boost::ref(y), 
																	 boost::ref(z), _1)));
			computation.push_back(new hyper::model::abortable_function(boost::bind(&times, 
																	 boost::ref(x), boost::ref(y), 
																	 boost::ref(z), _1)));

			computation.compute(boost::bind(&test_callback::handle_second_test, this, _1));
			handled_test++;
		}

		void test()
		{
			computation.push_back(new hyper::model::abortable_function(boost::bind(&add, 
																	 boost::ref(x), boost::ref(y), 
																	 boost::ref(z), _1)));
			computation.push_back(new hyper::model::abortable_function(boost::bind(&add, 
																	 boost::ref(x), boost::ref(y), 
																	 boost::ref(z), _1)));
			computation.push_back(new hyper::model::abortable_function(boost::bind(&times, 
																	 boost::ref(x), boost::ref(y), 
																	 boost::ref(z), _1)));
			computation.push_back(new hyper::model::abortable_function(boost::bind(&times, 
																	 boost::ref(x), boost::ref(y), 
																	 boost::ref(z), _1)));
			z = 0;
			x = 12;
			y = 3;

			computation.compute(boost::bind(&test_callback::handle_first_test, this, _1));
		}
	};
}

BOOST_AUTO_TEST_CASE ( model_abortable_function_test )
{
	test_callback test;
	test.handled_test = 0;
	test.test();
	BOOST_CHECK(test.handled_test == 2);
}
