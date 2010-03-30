#include <compiler/parser.hh>
#include <compiler/universe.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( compiler_parser_test )
{
	using namespace hyper::compiler;

	bool r;
	universe u;
	parser P(u);
	r = P.parse_ability_file("./example.ability");
	r = P.parse_expression("42");
	r = P.parse_expression("42.0");
	r = P.parse_expression("\"a wild string\"");
	r = P.parse_expression("toto");
	r = P.parse_expression("f");
	r = P.parse_expression("f ()");
	r = P.parse_expression("f(g(42), 42.0, k(\"some string\"), l(i, j))");
	r = P.parse_expression("true");
	r = P.parse_expression("g(false)");
	r = P.parse_expression("-42");
	r = P.parse_expression("+42");
	r = P.parse_expression("42 * 3");
	r = P.parse_expression("42 + 14 * 3");
	r = P.parse_expression("var1 + var2");
	r = P.parse_expression("(var1 + var2) * -6");
	r = P.parse_expression("f(g(42)) * l(12+3)");
	r = P.parse_expression("42 < 3");
	r = P.parse_expression("(42 > 3) == (14*3 > 2)");
	r = P.parse_expression("(42 > 3) == (14*3 > 2) && f(42) > 11");
	r = P.parse_expression("((42 > 3) == (14*3 > 2)) && (f(42) > 11)");
	r = P.parse_expression("a && b || c");
	r = P.parse_expression("a || b && c");
}
