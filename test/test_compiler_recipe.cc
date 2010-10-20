#include <hyperConfig.hh>

#include <compiler/ability.hh>
#include <compiler/task.hh>
#include <compiler/task_parser.hh>
#include <compiler/parser.hh>
#include <compiler/recipe.hh>
#include <compiler/recipe_parser.hh>
#include <compiler/universe.hh>
#include <boost/test/unit_test.hpp>

using namespace hyper::compiler;

namespace {
	struct recipe_test 
	{
		const universe& u;
		parser& P;
		const ability &ab;
		task t;

		recipe_test(const universe& u_, parser& P_, const ability& ab_):
			u(u_), P(P_), ab(ab_), t(task_decl(), ab, u.types())
		{}

		void do_build_test(const std::string& recipe_descr, bool expect_valid)
		{
			recipe_decl_list r_list;
			BOOST_CHECK( P.parse_recipe(recipe_descr, r_list));
			BOOST_CHECK( r_list.recipes.size() == 1);

			recipe rec(r_list.recipes[0], ab, t, u.types());
			BOOST_CHECK( rec.validate(u) == expect_valid );
		}
	};
}


BOOST_AUTO_TEST_CASE ( compiler_recipe_test )
{

	universe u;
	parser P(u);
	BOOST_CHECK( P.parse_ability_file("./example.ability") == true );

	recipe_test check_recipe(u, P, u.get_ability("first"));

	check_recipe.do_build_test("r0 = recipe { pre = {}; post = {}; body = {}; };", true);

	check_recipe.do_build_test("r1 = recipe { pre = {}; post = {}; body = {  make (2 == 1); }; };",
								true);

	check_recipe.do_build_test("r2 = recipe { pre = {}; \
											   post = {};  \
											   body = {  make ((first::i == 1)); }; };",
								true);

	check_recipe.do_build_test("r3 = recipe { pre = {};	\
											   post = {};   \
											   body = {  ensure ((first::i == 1)); }; };",
								true);


	check_recipe.do_build_test("r4 = recipe { pre = {};	\
											   post = {};   \
											   body = {  let Z ensure ((first::i == 1)); \
														 abort Z; \
													  }; };",
								true);
	
	check_recipe.do_build_test("r5 = recipe { pre = {};	\
											   post = {};   \
											   body = {  let s square(2.0);; \
													  }; };",
								true);

	/* z is inferred as double, can't call + between an int and a double */
	check_recipe.do_build_test("r6 = recipe { pre = {}; \
											  post = {}; \
											  body = { let s square(2.0); \
													   s +  first::i; \
													 }; };",
								false);

	check_recipe.do_build_test("r7 = recipe { pre = {}; \
											  post = {}; \
											  body = { let s square(2.0); \
													   let s2 square(s); \
													 }; };",
								true);

	/* Can't declare i in local scope, as it will shadow first::i */
	check_recipe.do_build_test("r8 = recipe { pre = {};	\
											   post = {};   \
											   body = {  let i square(2.0);; \
													  }; };",
								false);


	/* Can't use twice the same variable name in local scope */
	check_recipe.do_build_test("r9 = recipe { pre = {}; \
											  post = {}; \
											  body = { let s square(2.0); \
													   let s square(3.0); \
													 }; };",
								false);

	/* abort expect an idenfitier in input */
	check_recipe.do_build_test("r10 = recipe { pre = {}; \
											  post = {}; \
											  body = { let s square(2.0); \
													   abort s;			  \
													 }; };",
								false);

	/* make takes a boolean expression in input */
	check_recipe.do_build_test("r11 = recipe { pre = {}; post = {};  \
										body = {  make ( square(2.0) ); }; };",
								false);

	check_recipe.do_build_test("r12 = recipe { pre = {}; post = {};  \
										body = {  make ( square(2.0) < 2.0 ); }; };",
								true);

	/* wait takes a boolean expression in input */
	check_recipe.do_build_test("r13 = recipe { pre = {}; post = {};  \
										body = {  wait ( square(2.0) ); }; };",
								false);

	check_recipe.do_build_test("r14 = recipe { pre = {}; post = {};  \
										body = { wait ( square(2.0) < 2.0 ); }; };",
								true);
	

	/* wait returns void, and let doesn't accept to bind void thing */
	check_recipe.do_build_test("r15 = recipe { pre = {}; post = {};  \
										body = {  let X wait ( square(2.0) < 2.0 ); }; };",
								false);

	/* ensure takes a boolean expression in input */
	check_recipe.do_build_test("r16 = recipe { pre = {}; post = {};  \
										body = {  ensure ( square(2.0) ); }; };",
								false);

	check_recipe.do_build_test("r17 = recipe { pre = {}; post = {};  \
										body = { ensure ( square(2.0) < 2.0 ); }; };",
								true);

	/* A more complex beast */
	check_recipe.do_build_test("r18 = recipe { pre = {}; post = {};  \
										body = {					 \
											let pi 3.1415;			 \
											let heuristic (square(thresold) * pi); \
											make ( first::myPrivatevariable < heuristic) \
											let X ensure ( square(2.0) < 2.0 ); \
											square(pi);				\
											/* more computation */	\
											square(pi);				\
											/* abort ensure clause */ \
											abort X; }; };",
								true);
}
