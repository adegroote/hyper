#include <logic/engine.hh>
#include <logic/eval.hh>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/variant/apply_visitor.hpp>

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

	BOOST_CHECK(e.add_type("int"));
	BOOST_CHECK(e.add_type("double"));
	BOOST_CHECK(e.add_type("point"));

	BOOST_CHECK(e.add_predicate("less_int", 2, boost::assign::list_of("int")("int"), new eval<less, 2>()));
	BOOST_CHECK(e.add_predicate("less_double", 2, boost::assign::list_of("double")("double"), new eval<less, 2>()));

	BOOST_CHECK(e.add_func("distance", 2, boost::assign::list_of("point")("point")("double")));

	BOOST_CHECK(e.add_rule<std::string>("less_int_transitiviy", 
						   boost::assign::list_of<std::string>("less_int(X, Y)")("less_int(Y,Z)"),
						   boost::assign::list_of<std::string>("less_int(X, Z)")));

	BOOST_CHECK(e.add_rule<std::string>("less_double_transitiviy", 
						   boost::assign::list_of<std::string>("less_double(X, Y)")("less_double(Y,Z)"),
						   boost::assign::list_of<std::string>("less_double(X, Z)")));

	BOOST_CHECK(e.add_rule<std::string>("distance_symmetry",
						   boost::assign::list_of<std::string>("distance(A,B)"),
						   boost::assign::list_of<std::string>("equal_double(distance(A,B), distance(B,A))")));

	BOOST_CHECK(e.add_rule<std::string>("less_int_false",
						   boost::assign::list_of<std::string>("less_int(A, A)"),
						   std::vector<std::string>()));

	BOOST_CHECK(e.add_rule<std::string>("less_double_false",
						   boost::assign::list_of<std::string>("less_double(A, A)"),
						   std::vector<std::string>()));

	BOOST_CHECK(e.add_rule<std::string>("less_int_antisymetry",
						   boost::assign::list_of<std::string>("less_int(A, B)")("less_int(B,A)"),
						   std::vector<std::string>()));

	BOOST_CHECK(e.add_rule<std::string>("less_double_antisymetry",
						   boost::assign::list_of<std::string>("less_double(A, B)")("less_double(B,A)"),
						   std::vector<std::string>()));

	BOOST_CHECK(e.add_fact("equal_int(x, y)"));
	BOOST_CHECK(e.add_fact("equal_int(x, 7)"));
	BOOST_CHECK(e.add_fact("less_int(y, 9)"));
	BOOST_CHECK(e.add_fact("less_int(z, y)"));
	
	BOOST_CHECK(e.add_fact("less_double(distance(center, object), 3.0)"));
	BOOST_CHECK(e.add_fact("equal_point(object, balloon)"));

	// direct inconsistency with rules "less_false"
	BOOST_CHECK(!e.add_fact("less_int(h, h)"));

	BOOST_CHECK(e.add_fact("equal_int(a, 9)"));

	// inconsistent by deduction
	BOOST_CHECK(!e.add_fact("less_int(a, x)"));

	BOOST_CHECK(e.add_fact("equal_int(b, 10)"));

	// inconsistent by deduction / execution
	BOOST_CHECK(!e.add_fact("less_int(b, x)"));


	boost::logic::tribool r;

	r = e.infer("equal_int(x, 7)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	// reflexivity
	r = e.infer("equal_int(7, x)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	// reflexivity + transitivity
	r = e.infer("equal_int(7, y)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	// forward chaining less(z, 12) true if less(z,9) and less(9, 12)
	r = e.infer("less_int(z, 12)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	// y < 9 and x = y
	r = e.infer("less_int(x, 12)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	r = e.infer("less_double(distance(center, object), 5.0)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	r = e.infer("less_double(distance(center, balloon), 5.0)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);
	std::cout << e << std::endl;

	r = e.infer("less_double(distance(object, center), 8.0)");
	BOOST_CHECK(! boost::logic::indeterminate(r));
	BOOST_CHECK(r);

	r = e.infer("less_double(distance(object, center), 1.0)");
	BOOST_CHECK(boost::logic::indeterminate(r));

	r = e.infer("less_double(distance(object, x), 2.0)");
	BOOST_CHECK(boost::logic::indeterminate(r));


	/* Add some fact for task1 */
	BOOST_CHECK(e.add_fact("equal_int(x, y)", "task1"));
	BOOST_CHECK(e.add_fact("equal_int(x, 7)", "task1"));

	/* Add some fact for task2 */
	BOOST_CHECK(e.add_fact("equal_int(x, 7)", "task2"));

	std::vector<std::string> res;
	e.infer_all("equal_int(y, 7)", std::back_inserter(res));


	BOOST_CHECK(res.size() == 2);
	BOOST_CHECK(res[0] == "default");
	BOOST_CHECK(res[1] == "task1");

	res.clear();

	e.infer_all("less_double(distance(center, object), 5.0)", std::back_inserter(res));
	BOOST_CHECK(res.size() == 1);
	BOOST_CHECK(res[0] == "default");

	res.clear();
	e.infer_all("less_double(distance(center, object), 1.0)", std::back_inserter(res));
	BOOST_CHECK(res.size() == 0);

	/* Testing infer with suggestion, creating new context */
	BOOST_CHECK(e.add_fact("less_double(distance(center, object), 3.0)", "task3"));
	std::vector<function_call> hyps;
	r = e.infer("less_double(distance(center, object), treshold)", hyps, std::string("task3"));
	BOOST_CHECK(boost::logic::indeterminate(r));
	BOOST_CHECK(hyps.size() == 1);
	generate_return r1 = generate("less_double(3.0, treshold)", e.funcs());
	BOOST_CHECK(hyps[0] == r1.e);

	BOOST_CHECK(e.add_fact("equal_int(x, 4)", "task3"));
	r = e.infer("equal_int(4, y)", hyps, std::string("task3"));

	BOOST_CHECK(hyps.size() > 1);
	r1 = generate("equal_int(x,y)", e.funcs());
	BOOST_CHECK(std::find(hyps.begin(), hyps.end(), r1.e) != hyps.end());
}

