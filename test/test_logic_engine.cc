#include <logic/engine.hh>
#include <logic/eval.hh>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>

using namespace boost::logic;
using namespace hyper::logic;

namespace {
struct are_equal : public boost::static_visitor<tribool>
{
	template <typename T, typename U>
	tribool operator()(const T& t, const U& u) const
	{
		(void)t; (void) u; 
		return boost::logic::indeterminate;
	}

	template <typename U>
	tribool operator () (const Constant<U>& u, const Constant<U>& v) const
	{
		return (u.value == v.value);
	}
};

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

struct equal
{
	tribool operator() (const expression& e1, const expression& e2) const
	{
		return boost::apply_visitor(are_equal(), e1.expr, e2.expr);
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

	BOOST_CHECK(e.add_func("equal", 2, new eval<equal, 2>()));
	BOOST_CHECK(e.add_func("less", 2, new eval<less, 2>()));

	BOOST_CHECK(e.add_rule("equal_reflexivity", 
						   boost::assign::list_of<std::string>("equal(X, Y)"),
						   boost::assign::list_of<std::string>("equal(Y, X)")));

	BOOST_CHECK(e.add_rule("equal_transitiviy", 
						   boost::assign::list_of<std::string>("equal(X, Y)")("equal(Y,Z)"),
						   boost::assign::list_of<std::string>("equal(X, Z)")));

	BOOST_CHECK(e.add_rule("less_transitiviy", 
						   boost::assign::list_of<std::string>("less(X, Y)")("less(Y,Z)"),
						   boost::assign::list_of<std::string>("less(X, Z)")));

	BOOST_CHECK(e.add_fact("equal(x, y)"));
	BOOST_CHECK(e.add_fact("equal(x, 7)"));
	BOOST_CHECK(e.add_fact("less(y, 9)"));
	BOOST_CHECK(e.add_fact("less(z, y)"));

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

	r = e.infer("less(z, 12)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);
}

