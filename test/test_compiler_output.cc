#include <compiler/output.hh>
#include <boost/test/unit_test.hpp>
#include <boost/assign/std/vector.hpp> 

struct dump {
	std::ostream& oss;

	dump(std::ostream& oss) : oss(oss) {}

	void operator() (int i) { oss << i; }

	void separator() { oss << ", "; }
};

BOOST_AUTO_TEST_CASE ( compiler_output )
{
	using namespace boost::assign; // bring 'operator+=()' into scope
	std::vector<int> v1;

	std::ostringstream oss;

	hyper::compiler::list_of(v1.begin(), v1.end(), dump(oss));
	BOOST_CHECK(oss.str() == "");

	v1+= 1, 2, 3, 4, 5;
	hyper::compiler::list_of(v1.begin(), v1.end(), dump(oss));
	BOOST_CHECK(oss.str() == "1, 2, 3, 4, 5");
}
