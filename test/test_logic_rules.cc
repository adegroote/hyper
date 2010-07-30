#include <logic/rules.hh>
#include <boost/test/unit_test.hpp>

#include <boost/assign/list_of.hpp>

BOOST_AUTO_TEST_CASE ( logic_rules_test)
{
	using namespace hyper::logic;

	funcDefList funcs;
	rules r(funcs);

	funcs.add("equal", 2);

	bool res;
	res = r.add(
		"reflexivity",
		boost::assign::list_of<std::string>("equal(X,Y)"),
		boost::assign::list_of<std::string>("equal(Y, X)")
	);

	BOOST_CHECK(res);
	BOOST_CHECK(r.size() == 1);

	res = r.add(
		"transitivity",
		boost::assign::list_of<std::string>("equal(X, Y)")("equal(Y, Z)"),
		boost::assign::list_of<std::string>("equal(X, Z)")
	);

	BOOST_CHECK(res);
	BOOST_CHECK(r.size() == 2);

	std::copy(r.begin(), r.end(), std::ostream_iterator<rule>(std::cout, "\n"));
}
