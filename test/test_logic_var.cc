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
	bool operator() (const U&) const { return false; }

	bool operator() (const T&) const { return true; }
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
	adapt_res res = db.adapt_and_unify(r.e);
	BOOST_CHECK(expected_answer<T>(res));
	return res;
}
}

BOOST_AUTO_TEST_CASE ( logic_logic_var_test )
{
	funcDefList funcs;
	logic_var_db db(funcs);

	funcs.add("equal", 2, new eval<equal, 2>(), true);
	funcs.add("less", 2, new eval<less, 2>());
	funcs.add("distance", 2);

	adapt_res res;

	do_test<adapt_res::ok>("equal(x, 7)", funcs, db);
	do_test<adapt_res::ok>("equal(y, z)", funcs, db);
	res = do_test<function_call>("less(z, 9)", funcs, db);
	function_call f = boost::get<function_call>(res.res);

	std::vector<function_call> v_f = db.deadapt(f);
	generate_return r1 = generate("less(y, 9)", funcs);
	generate_return r2 = generate("less(z, 9)", funcs);
	BOOST_CHECK(v_f.size() == 2);
	BOOST_CHECK(std::find(v_f.begin(), v_f.end(), r1.e) != v_f.end());
	BOOST_CHECK(std::find(v_f.begin(), v_f.end(), r2.e) != v_f.end());

	// unify logic reprensation of x and y
	do_test<adapt_res::require_permutation>("equal(x, y)", funcs, db);
	// nothing new
	do_test<adapt_res::ok>("equal(x, z)", funcs, db);
	do_test<function_call>("less(distance(pt1, pt2), 3.0)", funcs, db);
	do_test<function_call>("less(distance(pt3, pt4), 5.0)", funcs, db);

	// unify logic representation of pt3 and pt1
	do_test<adapt_res::require_permutation>("equal(pt3, pt1)", funcs, db);

	// unify logic representation of pt4 and pt2 and distance(pt1, pt3)
	do_test<adapt_res::require_permutation>("equal(pt4, pt2)", funcs, db);

	// check deadapt with recursive case
	res = do_test<function_call>("less(distance(pt3, pt4), 5.0)", funcs, db);
	f = boost::get<function_call>(res.res);
	v_f = db.deadapt(f);
	r1 = generate("less(distance(pt3, pt4), 5.0)", funcs);
	r2 = generate("less(distance(pt3, pt2), 5.0)", funcs);
	generate_return r3 = generate("less(distance(pt1, pt4), 5.0)", funcs);
	generate_return r4 = generate("less(distance(pt1, pt2), 5.0)", funcs);
	BOOST_CHECK(v_f.size() == 4);
	BOOST_CHECK(std::find(v_f.begin(), v_f.end(), r1.e) != v_f.end());
	BOOST_CHECK(std::find(v_f.begin(), v_f.end(), r2.e) != v_f.end());
	BOOST_CHECK(std::find(v_f.begin(), v_f.end(), r3.e) != v_f.end());
	BOOST_CHECK(std::find(v_f.begin(), v_f.end(), r4.e) != v_f.end());

	// x unified with y, but x is already bounded to 7
	do_test<adapt_res::conflicting_facts>("equal(y, 8)", funcs, db);

	do_test<adapt_res::ok>("equal(i, 0)", funcs, db);

	// can't unify i with z, as they have different values
	do_test<adapt_res::conflicting_facts>("equal(i, z)", funcs, db);


}
