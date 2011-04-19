#include <logic/rules.hh>
#include <boost/test/unit_test.hpp>

#include <boost/assign/list_of.hpp>

BOOST_AUTO_TEST_CASE ( logic_rules_test)
{
	using namespace hyper::logic;

	funcDefList funcs;
	rules r(funcs);

	funcs.add("equal", 2);
	funcs.add("less", 2);

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

	// a conclusion with no condition is something always true
	res = r.add(
		"truth",
		std::vector<std::string>(),
		boost::assign::list_of<std::string>("equal(X, X)"));

	BOOST_CHECK(res);
	BOOST_CHECK(r.size() == 3);

	// a condition with no conclusion means that condition is false
	res = r.add(
		"false",
		boost::assign::list_of<std::string>("less(X, X)"),
		std::vector<std::string>());

	BOOST_CHECK(res);
	BOOST_CHECK(r.size() == 4);

	std::copy(r.begin(), r.end(), std::ostream_iterator<rule>(std::cout, "\n"));
}
