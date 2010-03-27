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
}
