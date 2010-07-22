#include <logic/expression.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( logic_expression_test)
{
	using namespace hyper::logic;

	funcDefList list;
	list.add("equal", 2);
	list.add("less", 2);
	list.add("void", 0);
	list.add("not", 1);

	generate_return r;
	r = generate("void()", list);
	BOOST_CHECK(r.res == true);

	r = generate("not(pipo)", list);
	BOOST_CHECK(r.res == true);

	r = generate("equal(x, y)", list);
	BOOST_CHECK(r.res == true);

	r = generate("less(x::value, y::value)", list);
	BOOST_CHECK(r.res == true);

	r = generate("pipo(x)", list);
	BOOST_CHECK(r.res == false);

	r = generate("equal(x, y, z)", list);
	BOOST_CHECK(r.res == false);
}
