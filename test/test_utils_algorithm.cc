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
}
