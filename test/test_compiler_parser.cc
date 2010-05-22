#include <compiler/parser.hh>
#include <compiler/universe.hh>
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE ( compiler_parser_test )
{
	using namespace hyper::compiler;

	bool r;
	universe u;
	parser P(u);
	BOOST_CHECK( P.parse_ability_file("./example.ability") == true );
	BOOST_CHECK( P.parse_expression("42") == true );
	BOOST_CHECK( P.parse_expression("42.0") == true );
	BOOST_CHECK( P.parse_expression("\"a wild string\"") == true );
	BOOST_CHECK( P.parse_expression("toto") == true );
	BOOST_CHECK( P.parse_expression("f") == true );
	BOOST_CHECK( P.parse_expression("f ()") == true );
	BOOST_CHECK( P.parse_expression("f(g(42), 42.0, k(\"some string\"), l(i, j))") == true );
	BOOST_CHECK( P.parse_expression("true") == true );
	BOOST_CHECK( P.parse_expression("g(false)") == true );
	BOOST_CHECK( P.parse_expression("-42") == true );
	BOOST_CHECK( P.parse_expression("+42") == true );
    BOOST_CHECK( P.parse_expression("42 * 3") == true );
    BOOST_CHECK( P.parse_expression("42 + 14 * 3") == true );
	BOOST_CHECK( P.parse_expression("var1 + var2") == true );
	BOOST_CHECK( P.parse_expression("(var1 + var2) * -6") == true );
	BOOST_CHECK( P.parse_expression("f(g(42)) * l(12+3)") == true );
    BOOST_CHECK( P.parse_expression("42 < 3") == true );
    BOOST_CHECK( P.parse_expression("(42 > 3) == (14*3 > 2)") == true );
    BOOST_CHECK( P.parse_expression("(42 > 3) == (14*3 > 2) && f(42) > 11") == true );
	BOOST_CHECK( P.parse_expression("((42 > 3) == (14*3 > 2)) && (f(42) > 11)") == true );
	BOOST_CHECK( P.parse_expression("a && b || c") == true );
	BOOST_CHECK( P.parse_expression("a || b && c") == true );
	BOOST_CHECK( P.parse_expression("toto::titi") == true );
	BOOST_CHECK( P.parse_expression("(pos::computeDistance(Dtm::lastMerged, Pos::currentPosition)"
									"< threshold) && (Path3D::goal == currentGoal)") == true );
	BOOST_CHECK( P.parse_task("toto = task { pre = {}; post = {}; };") == true );
	BOOST_CHECK( P.parse_task("titi = task { pre = { {init != false} {size < 42} };"
							                 "post = {{a && b || c}}; };") == true );
}
