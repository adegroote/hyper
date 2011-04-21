#include <logic/logic_var.hh>
#include <logic/eval.hh>
#include <boost/test/unit_test.hpp>
#include <boost/variant.hpp>

#include <iostream>

using namespace hyper::logic;
using namespace boost::logic;

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

template <typename T>
struct correct_type : boost::static_visitor<bool>
{
	template <typename U>
	bool operator() (const U& u) const { return false; }

	bool operator() (const T& t) const { return true; }
};

template <typename T>
bool expected_answer(const adapt_res& res)
{
	return boost::apply_visitor(correct_type<T>(), res.res);
}

template <typename T>
adapt_res do_test(const std::string& s, const funcDefList& funcs, logic_var_db& db)
{
	std::cerr << "logic_var::do_test " << s << std::endl;
	generate_return r = generate(s, funcs);
	BOOST_CHECK(r.res);
	adapt_res res = db.adapt(r.e);
	BOOST_CHECK(expected_answer<T>(res));
	return res;
}
}

BOOST_AUTO_TEST_CASE ( logic_logic_var_test )
{
	funcDefList funcs;
	logic_var_db db;

	funcs.add("equal", 2, new eval<equal, 2>());
	funcs.add("less", 2, new eval<less, 2>());
	funcs.add("distance", 2);

	do_test<adapt_res::ok>("equal(x, 7)", funcs, db);
	do_test<adapt_res::ok>("equal(y, z)", funcs, db);
	do_test<function_call>("less(z, 9)", funcs, db);

	// unify logic reprensation of x and y
	do_test<adapt_res::permutationSeq>("equal(x, y)", funcs, db);
	do_test<function_call>("less(distance(pt1, pt2), 3.0)", funcs, db);
	do_test<function_call>("less(distance(pt3, pt4), 5.0)", funcs, db);

	// unify logic representation of pt3 and pt1
	do_test<adapt_res::permutationSeq>("equal(pt3, pt1)", funcs, db);

	// unify logic representation of pt4 and pt2 and distance(pt1, pt3)
	do_test<adapt_res::permutationSeq>("equal(pt4, pt2)", funcs, db);

	// x unified with y, but x is already bounded to 7
	do_test<adapt_res::conflicting_facts>("equal(y, 8)", funcs, db);

	do_test<adapt_res::ok>("equal(i, 0)", funcs, db);

	// can't unify i with z, as they have different values
	do_test<adapt_res::conflicting_facts>("equal(i, z)", funcs, db);
}
