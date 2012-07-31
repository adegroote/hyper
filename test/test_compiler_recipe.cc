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

			recipe rec(r_list.recipes[0], r_list.context, ab, t, u.types());
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

	/* make and ensure expect that a remote name appears in the expression */
	check_recipe.do_build_test("r1 = recipe { pre = {}; post = {}; body = {  make (2 == 1); }; };",
								false);
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
										body = {  make ( square(other::thresold) < 2.0 ); }; };",
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
										body = { ensure ( square(other::thresold) < 2.0 ); }; };",
								true);

	/* A more complex beast */
	check_recipe.do_build_test("r18 = recipe { pre = {}; post = {};  \
										body = {					 \
											let pi 3.1415;			 \
											let heuristic (square(thresold) * pi); \
											make ( first::myPrivatevariable < heuristic) \
											let X ensure ( square(other::thresold) < 2.0 ); \
											square(pi);				\
											/* more computation */	\
											square(pi);				\
											/* abort ensure clause */ \
											abort X; }; };",
								true);

	/* set accepts any valid expression in input, as long as the type matches
	 * You can only set a local variable (from recipe up to ability)
	 */

	check_recipe.do_build_test("r19 = recipe { pre = {}; post = {}; \
									  body = {						\
										set first::isOk false;		\
										set i 12;					\
										let X i+j;					\
										set X X+1;					\
										};};", true);

	/* Can't assign a int to a bool */
	check_recipe.do_build_test("r20 = recipe { pre = {}; post = {}; \
									  body = {						\
										set first::isOk 22			\
										};};", false);

	/* square returns a double, can't assign a double to a bool */
	check_recipe.do_build_test("r21 = recipe { pre = {}; post = {}; \
									  body = {						\
										set first::isOk square(thresold) \
										};};", false);

	/* set on a remote variable is forbidden */
	check_recipe.do_build_test("r22 = recipe { pre = {}; post = {}; \
									  body = {						\
										set other::isEmpty  false\
										};};", false);
	
	/* Check the =>> operator, to handle "sequence" of constraint */
	check_recipe.do_build_test("r23 = recipe { pre = {}; post = {}; \
											   body = {					  \
													ensure(				  \
													first::isOk == true =>> \
													first::thresold < 2.0  \
													)						\
											    };							\
											  };", true);

	/* tagged function can only be used at the first element of an expression */
	check_recipe.do_build_test("r24 = recipe { pre = {}; post = {};			\
											   body = {						\
												genomTagTest(xxx, 22.3);	\
												};							\
											};", true);			

	check_recipe.do_build_test("r25 = recipe { pre = {}; post = {};			\
											   body = {						\
												square(						\
													genomTagTest(xxx, 22.3)	\
													);						\
												};							\
											};", false);			

	/* checking where syntax and validity */
	check_recipe.do_build_test("r26 = recipe { pre = {}; post = {};			\
											   body = {						\
											   make(first::isOk == true     \
												    where first::test_where == 2.0 \
												    )						\
												};							\
											};", true);			

	check_recipe.do_build_test("r27 = recipe { pre = {}; post = {};			\
											   body = {						\
											   make(first::isOk == true     \
												    where first::test_where ==  other::thresold\
												    )						\
												};							\
											};", true);			

	check_recipe.do_build_test("r28 = recipe { pre = {}; post = {};			\
											   body = {						\
											   make(first::isOk == true     \
												    where first::test_where ==  other::isEmpty\
												    )						\
												};							\
											};", false);			

	check_recipe.do_build_test("r28 = recipe { pre = {}; post = {};			\
											   body = {						\
											   make(first::isOk == true     \
												    where first::test_where ==  unknownVariable\
												    )						\
												};							\
											};", false);			

	check_recipe.do_build_test("r29 = recipe { pre = {}; post = {};			\
											   body = {						\
											   make(first::isOk == true     \
												    where other::isEmpty ==  false\
												    )						\
												};							\
											};", false);			

	/* Add some context */
	check_recipe.do_build_test("letname first_ctr first::isOk == true \
							    r30 = recipe { pre = {}; post = {};	  \
								body = {							  \
									make(first_ctr)					  \
									}								  \
								};", true);

	check_recipe.do_build_test("letname first_ctr first::isOk == true \
							    r31 = recipe { pre = {}; post = {};	  \
								body = {							  \
									make(first_ctr where first::test_where == 2.0)	 \
									}								  \
								};", true);

	check_recipe.do_build_test("letname first_ctr first::isOk == true	\
								let f y = fun { make(first_ctr where first::test_where == y) } \
							    r32 = recipe { pre = {}; post = {};	  \
								body = {							  \
										f(2.0)						  \
									}								  \
								};", true);

	check_recipe.do_build_test("letname first_ctr first::isOk == true	\
								let f y = fun { make(first_ctr where first::test_where == y) } \
								let g x = fun { let z x; f(z) }		  \
							    r33 = recipe { pre = {}; post = {};	  \
								body = {							  \
										let z 3.0					  \
										g(z)						  \
									}								  \
								};", true);

	check_recipe.do_build_test("letname first_ctr first::isOk == true \
								r34 = recipe { pre = {{ last_error?(first_ctr) }} \
											   post = {} body = {} };", true);

	check_recipe.do_build_test("letname first_ctr first::isOk == true \
								r35 = recipe { pre = {} \
											   post = {}					\
											   body = {						\
												while {first::isOk} {		\
													let z 3.0				\
													make(first::isOk == true where first::test_where == z) \
												}							\
												} };", true);

	/* Test the parsing of end primitive */
	check_recipe.do_build_test("r36 = recipe { pre = {}; post = {}; body = {}; end = {} };", true);
	check_recipe.do_build_test("r37 = recipe { pre = {}; post = {}; body = {}; end = { set first::isOk false} };", true);
	check_recipe.do_build_test("r37 = recipe { pre = {}; post = {}; body = {}; end = { set first::isOk 22.0 } };", false);

	/* Test the parsing of preference parameter */
	check_recipe.do_build_test("r38 = recipe { pre = {}; post = {}; prefer = 25; body = {}; };", true);

	/* assert tests */
	/* assert takes a boolean expression in input */
	check_recipe.do_build_test("r39 = recipe { pre = {}; post = {};  \
										body = {  assert ( square(2.0) ); }; };",
								false);

	check_recipe.do_build_test("r40 = recipe { pre = {}; post = {};  \
										body = { assert ( square(2.0) < 2.0 ); }; };",
								true);

	/* assert returns an identifier, so is assignable */
	check_recipe.do_build_test("r41 = recipe { pre = {}; post = {};  \
										body = { let X assert ( square(2.0) < 2.0 ); }; };",
								true);
}
