#include <logic/facts.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( logic_facts_test )
{
	using namespace hyper::logic;

	funcDefList funcs;
	facts our_facts(funcs);

	funcs.add("equal", 2);
	funcs.add("less", 2);

	our_facts.add("equal(x, 7)");
	our_facts.add("equal(x, y)");
	our_facts.add("less(z, 7)");

	generate_return r = generate("equal(x, 7)", funcs);
	BOOST_CHECK(r.res);

	/* Whoot so crazy inference here */
	BOOST_CHECK(our_facts.matches(r.e));
}
