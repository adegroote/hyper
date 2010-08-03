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
	unifyM ctx;
	res = unify(r2.e, r1.e, ctx);
	BOOST_CHECK(res.first == true);
	BOOST_CHECK(res.second.size() == 2);
	expression expected = std::string("x");
	BOOST_CHECK(res.second["a"] == expected);
	expression expected2 = Constant<int>(7);
	BOOST_CHECK(res.second["b"] == expected2);
	}

	unifyM ctx;
	res = unify(r1.e, r2.e, ctx);
	BOOST_CHECK(res.first == false);

	r1 = generate("less(x, \"pipo\")", list);
	BOOST_CHECK(r1.res);
	res = unify(r2.e, r1.e, ctx);
	BOOST_CHECK(res.first == false);

	{
	unifyM ctx;
	r1 = generate("equal(a, \"pipo\")", list);
	BOOST_CHECK(r1.res);
	res = unify(r2.e, r1.e, ctx);
	BOOST_CHECK(res.first == true);
	BOOST_CHECK(res.second.size() == 2);
	expression expected = std::string("a");
	BOOST_CHECK(res.second["a"] == expected);
	expression expected2 = Constant<std::string>("\"pipo\"");
	BOOST_CHECK(res.second["b"] == expected2);
	}

	{
	unifyM ctx;
	r1 = generate("equal(a, equal(x, y))", list);
	BOOST_CHECK(r1.res);
	res = unify(r2.e, r1.e, ctx);
	BOOST_CHECK(res.first == true);
	BOOST_CHECK(res.second.size() == 2);
	expression expected = std::string("a");
	BOOST_CHECK(res.second["a"] == expected);
	generate_return r3 = generate("equal(x, y)", list);
	BOOST_CHECK(r3.res);
	BOOST_CHECK(res.second["b"] == r3.e);

	res = unify(r1.e, r2.e, ctx);
	BOOST_CHECK(res.first == false);
	}

	// non-empty ctx
	r1 = generate("equal(x, 7)", list);
	r2 = generate("equal(a, b)", list);

	BOOST_CHECK(r1.res);
	BOOST_CHECK(r2.res);

	{
	unifyM ctx;
	expression e(std::string("x"));
	// adding a context for c doesn't change a lot
	ctx["c"] = e;
	res = unify(r2.e, r1.e, ctx);
	BOOST_CHECK(res.first == true);
	BOOST_CHECK(res.second.size() == 3);
	expression expected = std::string("x");
	BOOST_CHECK(res.second["a"] == expected);
	expression expected2 = Constant<int>(7);
	BOOST_CHECK(res.second["b"] == expected2);
	}

	{
	unifyM ctx;
	expression e(std::string("k"));
	// the cxt for a conflict with the computed unification, so fails
	ctx["a"] = e;
	res = unify(r2.e, r1.e, ctx);
	BOOST_CHECK(res.first == false);
	}

	{
	unifyM ctx;
	expression e(std::string("x"));
	// the cxt for a correspond to the computed unification, everything ok
	ctx["a"] = e;
	res = unify(r2.e, r1.e, ctx);
	BOOST_CHECK(res.second.size() == 2);
	expression expected = std::string("x");
	BOOST_CHECK(res.second["a"] == expected);
	expression expected2 = Constant<int>(7);
	BOOST_CHECK(res.second["b"] == expected2);
	}
	
	
}
