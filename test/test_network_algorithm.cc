#include <network/algorithm.hh>
#include <boost/assign/std/vector.hpp> 
#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>

using namespace hyper::network;
using namespace boost::assign;

namespace {
	typedef boost::function<void (const boost::system::error_code&)> cb_fun;

	struct double_ {
		void operator() (int& x, cb_fun cb) { x = x * 2;;  cb(boost::system::error_code()); }
	};

	struct algo_test {
		std::vector<int> v;

		algo_test() { 
			v+= 1, 2, 3, 4, 5;
		}

		void handle_second_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			std::vector<int> expected_res;
			expected_res+= 4, 8, 12, 16, 20;
			BOOST_CHECK(v == expected_res);
		}

		void handle_first_test(const boost::system::error_code& e) 
		{
			BOOST_CHECK(!e);
			std::vector<int> expected_res;
			expected_res+= 2, 4, 6, 8, 10;
			BOOST_CHECK(v == expected_res);
			async_parallel_for_each(v.begin(), v.end(), double_(),
						boost::bind(&algo_test::handle_second_test, this, boost::asio::placeholders::error));
		}

		void async_test() {
			async_for_each(v.begin(), v.end(), double_(), 
					boost::bind(&algo_test::handle_first_test, this, boost::asio::placeholders::error));
		}
	};
}

BOOST_AUTO_TEST_CASE ( network_algorithm_test )
{
	algo_test test;
	test.async_test();
}
