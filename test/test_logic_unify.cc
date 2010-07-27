#include <logic/unify.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( logic_unify_test )
{
	using namespace hyper::logic;

	funcDefList list;
	list.add("equal", 2);
	list.add("less", 2);

	generate_return r1, r2;
	r1 = generate("equal(x, 7)", list);
	r2 = generate("equal(a, b)", list);

	BOOST_CHECK(r1.res);
	BOOST_CHECK(r2.res);

	unify_res res;
	{
	res = unify(r2.e, r1.e);
	BOOST_CHECK(res.first == true);
	BOOST_CHECK(res.second.size() == 2);
	unify_p expected(std::string("a"), std::string("x"));
	BOOST_CHECK(res.second[0] == expected);
	unify_p expected2(std::string("b"), Constant<int>(7));
	BOOST_CHECK(res.second[1] == expected2);
	}

	res = unify(r1.e, r2.e);
	BOOST_CHECK(res.first == false);

	r1 = generate("less(x, \"pipo\")", list);
	BOOST_CHECK(r1.res);
	res = unify(r2.e, r1.e);
	BOOST_CHECK(res.first == false);

	{
	r1 = generate("equal(a, \"pipo\")", list);
	BOOST_CHECK(r1.res);
	res = unify(r2.e, r1.e);
	BOOST_CHECK(res.first == true);
	BOOST_CHECK(res.second.size() == 2);
	unify_p expected(std::string("a"), std::string("a"));
	BOOST_CHECK(res.second[0] == expected);
	unify_p expected2(std::string("b"), Constant<std::string>("\"pipo\""));
	BOOST_CHECK(res.second[1] == expected2);
	}

	{
	r1 = generate("equal(a, equal(x, y))", list);
	BOOST_CHECK(r1.res);
	res = unify(r2.e, r1.e);
	BOOST_CHECK(res.first == true);
	BOOST_CHECK(res.second.size() == 2);
	unify_p expected(std::string("a"), std::string("a"));
	BOOST_CHECK(res.second[0] == expected);
	generate_return r3 = generate("equal(x, y)", list);
	BOOST_CHECK(r3.res);
	unify_p expected2(std::string("b"), r3.e);
	BOOST_CHECK(res.second[1] == expected2);

	res = unify(r1.e, r2.e);
	BOOST_CHECK(res.first == false);
	}
	
}
