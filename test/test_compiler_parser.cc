#include <compiler/parser.hh>
#include <compiler/universe.hh>
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE ( compiler_parser_test )
{
	using namespace hyper::compiler;

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
	BOOST_CHECK( P.parse_expression("-42.0") == true );
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
	BOOST_CHECK( P.parse_task("in context first; tutu = task { pre = {{ other::isEmpty }}; "
															  "post = {}; };")
							  == true);
	// accessing private variable from other context is bad :)
	BOOST_CHECK( P.parse_task("in context first; x0 = task { pre = {{ other::myPrivatevariable }};"
													      " post = {}; };") == false);
	// in our local context, it is ok
	BOOST_CHECK( P.parse_task("in context first; x1 = task { pre = {{ first::myPrivatevariable }};"
													      " post = {}; };") == true);

	// calling unknow function is bad
	BOOST_CHECK( P.parse_task("in context first; x2 = task { pre = {{ toto() }};"
													      " post = {}; };") == false);

	// calling valid function with bad argument number is not really better
	BOOST_CHECK( P.parse_task("in context first; x3 = task { pre = {{ square() }};"
													      " post = {}; };") == false);

	// calling valid function with constant
	BOOST_CHECK( P.parse_task("in context first; x4 = task { pre = {{ square(42.0) }};"
													      " post = {}; };") == true);

	// calling valid function with variable of good type is ok
	BOOST_CHECK( P.parse_task("in context first; x5 = task { pre = {{ square(thresold) }};"
													      " post = {}; };") == true);

	// calling valid function with variable of bad type is bad
	BOOST_CHECK( P.parse_task("in context first; x6 = task { pre = {{ square(j) }};"
													      " post = {}; };") == false);

	// calling valid function with non-accessible variable is not really good
	BOOST_CHECK( P.parse_task("in context first; x7 = task { pre = {{ square(other::myPrivatevariable) }};"
													      " post = {}; };") == false);

	// calling operator < with two variables on same kind is ok
	BOOST_CHECK( P.parse_task("in context first; x8 = task { pre = {{ i < j }};"
													      " post = {}; };") == true);
	
	// calling operator < with variables of different kind is definitivly forbidden
	BOOST_CHECK( P.parse_task("in context first; x9 = task { pre = {{ i < 42.0 }};"
													      " post = {}; };") == false);

	// calling operator < with result of a function is ok as long as the two types match
	BOOST_CHECK( P.parse_task("in context first; x10 = task { pre = {{ square(thresold)  < 42.0 }};"
													      " post = {}; };") == true);

	// same kind of rules for == and !=
	BOOST_CHECK( P.parse_task("in context first; x11 = task { pre = {{ i == j }};"
													      " post = {}; };") == true);
	
	BOOST_CHECK( P.parse_task("in context first; x12 = task { pre = {{ i == 42.0 }};"
													      " post = {}; };") == false);

	BOOST_CHECK( P.parse_task("in context first; x13 = task { pre = {{ square(thresold) == 42.0 }};"
													      " post = {}; };") == true);

	// for operator && and ||, we only accept boolean expression
	BOOST_CHECK( P.parse_task("in context first; x14 = task { pre = {{ square(thresold)  &&  42.0 }};"
													      " post = {}; };") == false);

	BOOST_CHECK( P.parse_task("in context first; x15 = task { pre = {{ (i > j)  &&  (j > k) }};"
													      " post = {}; };") == true);

	// for operator -, only accept int and double in entry (probably not correct)
	BOOST_CHECK( P.parse_task("in context first; x16 = task { pre = {{ - (i < j) }};"
													      " post = {}; };") == false);

	BOOST_CHECK( P.parse_task("in context first; x17 = task { pre = {{ - square(42.0) }};"
													      " post = {}; };") == true);


	// you can't add two task with the same name
	BOOST_CHECK( P.parse_task("in context first; x17 = task { pre = {};"
													      " post = {}; };") == false);


	/* Testing recipe parsing */
	BOOST_CHECK( P.parse_recipe("r0 = recipe { pre = {}; post = {}; body = {}; };"));

	BOOST_CHECK( P.parse_recipe("r1 = recipe { pre = {}; post = {}; body = {  make (2 == 1); }; };"));

	BOOST_CHECK( P.parse_recipe("r2 = recipe { pre = {}; \
											   post = {};  \
											   body = {  make ((first::X == 1)); }; };"));

	BOOST_CHECK( P.parse_recipe("r3 = recipe { pre = {};	\
											   post = {};   \
											   body = {  ensure ((first::X == 1)); }; };"));


	BOOST_CHECK( P.parse_recipe("r4 = recipe { pre = {};	\
											   post = {};   \
											   body = {  let Z ensure ((first::X == 1)); \
														 abort Z; \
													  }; };"));
	
	BOOST_CHECK( P.parse_recipe("r5 = recipe { pre = {};	\
											   post = {};   \
											   body = {  let s square(2.0);; \
													  }; };"));

	std::string p3d_recipe = 
		"r6 = recipe { \
		pre = {};		\
		post = {};		\
		body = {		\
			ensure(pos::distance(dtm::lastMerge, pos::current) < 0.5 \
				&& p3d::goal == currentGoal \
				&& control::tracking == p3dSpeedRef \
				);									\
			wait(pos::distance(currentGoal, pos::current) < goalThreshold); \
			}; \
		}";

	BOOST_CHECK( P.parse_recipe(p3d_recipe));
}
