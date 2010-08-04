#include <logic/facts.hh>
#include <logic/eval.hh>
#include <boost/test/unit_test.hpp>

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


BOOST_AUTO_TEST_CASE ( logic_facts_test )
{
	using namespace hyper::logic;

	funcDefList funcs;
	facts our_facts(funcs);

	funcs.add("equal", 2, new eval<equal, 2>());
	funcs.add("less", 2, new eval<less, 2>());

	BOOST_CHECK(our_facts.size() == 0);
	our_facts.add("equal(x, 7)");
	BOOST_CHECK(our_facts.size() == 1);
	our_facts.add("equal(x, y)");
	BOOST_CHECK(our_facts.size() == 2);
	our_facts.add("less(z, 7)");
	BOOST_CHECK(our_facts.size() == 3);
	our_facts.add("equal(x, y)");
	// inserting a fact already present doesn't give you a lot more information
	BOOST_CHECK(our_facts.size() == 3);


	generate_return r = generate("equal(x, 7)", funcs);
	BOOST_CHECK(r.res);

	/* Whoot so crazy inference here */
	BOOST_CHECK(our_facts.matches(r.e));

	r = generate("equal(7, 9)", funcs);
	BOOST_CHECK(r.res);
	BOOST_CHECK(our_facts.matches(r.e) == false);

	r = generate("equal(7, 7)", funcs);
	BOOST_CHECK(r.res);
	BOOST_CHECK(our_facts.matches(r.e) == true);

	r = generate("less(7, 9)", funcs);
	BOOST_CHECK(r.res);
	BOOST_CHECK(our_facts.matches(r.e) == true);

	r = generate("less(7, 7)", funcs);
	BOOST_CHECK(r.res);
	BOOST_CHECK(our_facts.matches(r.e) == false);

	r = generate("less(z, 7)", funcs);
	BOOST_CHECK(r.res);
	BOOST_CHECK(our_facts.matches(r.e) == true);
	/* More difficult, equal is symetric */
	//BOOST_CHECK(our_facts.matches(r.e));

	r = generate("equal(v, w)", funcs);
	BOOST_CHECK(r.res);
	BOOST_CHECK(boost::logic::indeterminate( our_facts.matches(r.e)));
}
