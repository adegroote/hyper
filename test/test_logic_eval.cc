#include <logic/eval.hh>
#include <boost/test/unit_test.hpp>

#include <boost/logic/tribool.hpp>

using namespace boost::logic;
using namespace hyper::logic;

struct are_equal : public boost::static_visitor<tribool>
{
	template <typename T, typename U>
	tribool operator()(const T& t, const U& u) const
	{
		(void)t; (void) u; return boost::logic::indeterminate;
	}

	template <typename U>
	tribool operator () (const Constant<U>& u, const Constant<U>& v) const
	{
		return (u.value == v.value);
	}
};

struct equal
{
	tribool operator() (const expression& e1, const expression& e2) const
	{
		return boost::apply_visitor(are_equal(), e1.expr, e2.expr);
	}
};

struct not_equal 
{
	tribool operator() (const hyper::logic::expression& e1, const hyper::logic::expression& e2) const
	{
		return ! (equal()(e1, e2));
	}
};

BOOST_AUTO_TEST_CASE ( logic_eval_test )
{
	using namespace hyper::logic;

	funcDefList list;
	list.add("equal", 2);
	list.add("nequal", 2);

	generate_return r;
	r = generate("equal(7, 9)", list);
	BOOST_CHECK(r.res);
	tribool b = eval<equal, 2>() (r.e.args);
	BOOST_CHECK(!indeterminate(b));
	BOOST_CHECK(b == false);

	b = eval<notAPredicate, 0>() (r.e.args);
	BOOST_CHECK(indeterminate(b));

	b = eval<notAPredicate, 42>() (r.e.args);
	BOOST_CHECK(indeterminate(b));

	r = generate("equal(7, 7)", list);
	BOOST_CHECK(r.res);
	b = eval<equal, 2>()(r.e.args);
	BOOST_CHECK(!indeterminate(b));
	BOOST_CHECK(b == true);

	r = generate("equal(7, x)", list);
	BOOST_CHECK(r.res);
	b = eval<equal, 2>()(r.e.args);
	BOOST_CHECK(indeterminate(b));

	r = generate("nequal(7, 9)", list);
	BOOST_CHECK(r.res);
	b = eval<not_equal, 2>()(r.e.args);
	BOOST_CHECK(!indeterminate(b));
	BOOST_CHECK(b == true);


	r = generate("nequal(7, 7)", list);
	BOOST_CHECK(r.res);
	b = eval<not_equal, 2>()(r.e.args);
	BOOST_CHECK(!indeterminate(b));
	BOOST_CHECK(b == false);

	r = generate("nequal(7, x)", list);
	BOOST_CHECK(r.res);
	b = eval<not_equal, 2>()(r.e.args);
	BOOST_CHECK(indeterminate(b));

}
