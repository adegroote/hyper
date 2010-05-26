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
	BOOST_CHECK( P.parse_task("in context first; toto = task { pre = {}; post = {}; };") == true );
	// accessing non-existing symbol must fail
	BOOST_CHECK( P.parse_task("in context first; titi = task { pre = { {init} }; post = {}; };")
							  == false);
	// accessing local symbol, without scope is ok
	BOOST_CHECK( P.parse_task("in context first; titi = task { pre = {{ isOk }}; post = {}; };")
							  == true);
	// accessing remote symbol, if readable or controlable is ok
	BOOST_CHECK( P.parse_task("in context first; titi = task { pre = {{ other::isEmpty }}; "
															  "post = {}; };")
							  == true);
	// accessing private variable from other context is bad :)
	BOOST_CHECK( P.parse_task("in context first; x = task { pre = {{ other::myPrivatevariable }};"
													      " post = {}; };") == false);
	// in our local context, it is ok
	BOOST_CHECK( P.parse_task("in context first; x = task { pre = {{ first::myPrivatevariable }};"
													      " post = {}; };") == true);
}
