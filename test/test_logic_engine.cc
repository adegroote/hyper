#include <logic/engine.hh>
#include <logic/eval.hh>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>

using namespace boost::logic;
using namespace hyper::logic;

namespace {

struct is_less : public boost::static_visitor<tribool>
{
	template <typename T, typename U>
	tribool operator()(const T& t, const U& u) const
	{
		(void)t; (void) u; return boost::logic::indeterminate;
	}

	template <typename U>
	tribool operator () (const Constant<U>& u, const Constant<U>& v) const
	{
		return (u.value < v.value);
	}
};


struct less
{
	tribool operator() (const expression& e1, const expression& e2) const
	{
		return boost::apply_visitor(is_less(), e1.expr, e2.expr);
	}
};
}

BOOST_AUTO_TEST_CASE ( logic_engine_test )
{
	engine e;

	BOOST_CHECK(e.add_predicate("less", 2, new eval<less, 2>()));
	BOOST_CHECK(e.add_func("distance", 2));

	BOOST_CHECK(e.add_rule("less_transitiviy", 
						   boost::assign::list_of<std::string>("less(X, Y)")("less(Y,Z)"),
						   boost::assign::list_of<std::string>("less(X, Z)")));

	BOOST_CHECK(e.add_rule("distance_symmetry",
						   boost::assign::list_of<std::string>("distance(A,B)"),
						   boost::assign::list_of<std::string>("equal(distance(A,B), distance(B,A))")));

	BOOST_CHECK(e.add_fact("equal(x, y)"));
	BOOST_CHECK(e.add_fact("equal(x, 7)"));
	BOOST_CHECK(e.add_fact("less(y, 9)"));
	BOOST_CHECK(e.add_fact("less(z, y)"));
	BOOST_CHECK(e.add_fact("less(distance(center, object), 0.5)"));
	BOOST_CHECK(e.add_fact("equal(object, balloon)"));

	std::cout << e << std::endl;

	boost::logic::tribool r;
	r = e.infer("equal(x, 7)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	// reflexivity
	r = e.infer("equal(7, x)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	// reflexivity + transitivity
	r = e.infer("equal(7, y)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	// forward chaining less(z, 12) true if less(z,9) and less(9, 12)
	r = e.infer("less(z, 12)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	// y < 9 and x = y
	r = e.infer("less(x, 12)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	r = e.infer("less(distance(center, object), 1.0)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	r = e.infer("less(distance(center, balloon), 1.0)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	r = e.infer("less(distance(object, center), 1.0)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);
}

