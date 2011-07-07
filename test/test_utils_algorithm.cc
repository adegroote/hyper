#include <utils/algorithm.hh>
#include <boost/assign/std/vector.hpp> 
#include <boost/test/unit_test.hpp>

struct always_true {
	bool operator()(int) { return true; }
};

struct always_false { 
	bool operator()(int) { return false; }
};

bool is_even(int x) { return (x % 2 == 0); }

struct double_ {
	 int operator () (int value) { return 2 * value; }
};

BOOST_AUTO_TEST_CASE( utils_algorithm_test )
{
	using namespace boost::assign; // bring 'operator+=()' into scope

	std::vector<int> v1, v2;
	v1+= 2, 4, 6, 8, 10;

	
	BOOST_CHECK(hyper::utils::any(v1.begin(), v1.end(), 
								  std::bind2nd(std::equal_to<int>(), 2)));
	BOOST_CHECK(!hyper::utils::any(v1.begin(), v1.end(), 
								  std::bind2nd(std::equal_to<int>(), 3)));
	/* On an empty object, the any property is always false */
	BOOST_CHECK(!hyper::utils::any(v2.begin(), v2.end(), always_true()));

	BOOST_CHECK(hyper::utils::all(v1.begin(), v1.end(), &is_even));

	v1+= 13;

	BOOST_CHECK(!hyper::utils::all(v1.begin(), v1.end(), &is_even));

	/* On an empty object, the all property is always true */
	BOOST_CHECK(hyper::utils::all(v2.begin(), v2.end(), always_false()));

	std::vector<int> v, res, res2;
	v+= -2, -1, 0, 1, 2;

	hyper::utils::copy_if(v.begin(), v.end(), std::back_inserter(res), 
							 std::bind2nd(std::less<int>(), 0));

	BOOST_CHECK(res.size() == 2);
	BOOST_CHECK(res[0] == -2);
	BOOST_CHECK(res[1] == -1);

	hyper::utils::copy_if(v.begin(), v.end(), std::back_inserter(res2),
							 std::bind2nd(std::not_equal_to<int>(), 1));

	BOOST_CHECK(res2.size() == 4);
	BOOST_CHECK(res2[0] == -2);
	BOOST_CHECK(res2[1] == -1);
	BOOST_CHECK(res2[2] == 0);
	BOOST_CHECK(res2[3] == 2);

	v.clear();
	res.clear();
	v+= 0, 1, 2, 3, 4, 5;

	hyper::utils::transform_if(v.begin(), v.end(), std::back_inserter(res),
							   &is_even, double_());

	BOOST_CHECK(res.size() == 3);
	BOOST_CHECK(res[0] == 0);
	BOOST_CHECK(res[1] == 4);
	BOOST_CHECK(res[2] == 8);

}
