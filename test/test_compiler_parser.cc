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

	/* Basic expression with no unification clause clauses */
	BOOST_CHECK( P.parse_logic_expression("first::X == 1") == true );
	BOOST_CHECK( P.parse_logic_expression("loc::distance(pos::current, pos::goal) < 0.5") == true);
	BOOST_CHECK( P.parse_logic_expression("loc::distance(pos::current, pos::goal) < 0.5 where pos::goal == currentGoal") == true);
	BOOST_CHECK( P.parse_logic_expression("loc::distance(pos::current, pos::goal) < 0.5 \
													where pos::goal == currentGoal\
													    && pos::thresold == 0.2") == true);

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
											   body = {  let Z ensure (first::X == 1); \
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
			ensure((pos::distance(dtm::lastMerge, pos::current) < 0.5) \
				=>> (p3d::goal == currentGoal) \
				=>> (control::tracking == p3dSpeedRef) \
				);									\
			wait(pos::distance(currentGoal, pos::current) < goalThreshold); \
			}; \
		}";

	BOOST_CHECK( P.parse_recipe(p3d_recipe));

	std::string p3d_recipe2 = 
		"r7 = recipe { \
		pre = {};		\
		post = {};		\
		body = {		\
			ensure(pos::distance(dtm::lastMerge, pos::current) < 0.5 \
				=>> p3d::goal == currentGoal where p3d::goal == pos::current \
				=>> control::tracking == p3dSpeedRef \
				);									\
			wait(pos::distance(currentGoal, pos::current) < goalThreshold); \
			}; \
		}";

	BOOST_CHECK( P.parse_recipe(p3d_recipe2));
}
