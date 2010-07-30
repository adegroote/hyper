#include <logic/expression.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( logic_expression_test)
{
	using namespace hyper::logic;

	funcDefList list;
	list.add("equal", 2);
	list.add("less", 2);
	list.add("plus", 2);
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

	generate_return r1, r2, r3, r4, r5, r6;
	r1 = generate("equal(x, y)", list);
	BOOST_CHECK(r1.res);
	r2 = generate("equal(x, plus(7, 3))", list);
	BOOST_CHECK(r2.res);
	r3 = generate("less(x, \"toto\")", list);
	BOOST_CHECK(r3.res);
	r4 = generate("equal(x, y)", list);
	BOOST_CHECK(r4.res);
	r5 = generate("equal(x, plus(7, 3))", list);
	BOOST_CHECK(r5.res);
	r6 = generate("less(x, 2.6)", list);
	BOOST_CHECK(r6.res);

	BOOST_CHECK(r1.e != r2.e);
	BOOST_CHECK(r1.e != r3.e);
	BOOST_CHECK(r1.e == r4.e);
	BOOST_CHECK(r2.e == r5.e);
	BOOST_CHECK(r3.e != r4.e);

	BOOST_CHECK(r1.e < r2.e);
	BOOST_CHECK(r1.e < r3.e);
	BOOST_CHECK(! (r1.e < r4.e));
	BOOST_CHECK(! (r3.e < r6.e));
}
