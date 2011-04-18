#include <hyperConfig.hh>

#include <compiler/ability.hh>
#include <compiler/task.hh>
#include <compiler/parser.hh>
#include <compiler/task_parser.hh>
#include <compiler/universe.hh>
#include <boost/test/unit_test.hpp>

using namespace hyper::compiler;

namespace {
	struct task_test 
	{
		const universe& u;
		parser& P;
		const ability &ab;

		task_test(const universe& u_, parser& P_, const ability& ab_):
			u(u_), P(P_), ab(ab_)
		{}

		void do_build_test(const std::string& task_descr, bool expect_valid)
		{
			task_decl_list t_list;
			BOOST_CHECK( P.parse_task(task_descr, t_list));
			BOOST_CHECK( t_list.list.size() == 1);

			task t(t_list.list[0], ab, u.types());
			BOOST_CHECK( t.validate(u) == expect_valid );
		}
	};
}

BOOST_AUTO_TEST_CASE ( compiler_task_test )
{
	universe u;
	parser P(u);
	BOOST_CHECK( P.parse_ability_file("./example.ability") == true );

	task_test check_task(u, P, u.get_ability("first"));

	// accessing non-existing symbol must fail
	check_task.do_build_test("titi = task { pre = { {init} }; post = {}; };",
							  false);

	// accessing local symbol, without scope is ok
	check_task.do_build_test("titi = task { pre = {{ isOk }}; post = {}; };",
							  true);
	// accessing remote symbol, if readable or controlable is ok
	check_task.do_build_test("tutu = task { pre = {{ other::isEmpty }}; "
															  "post = {}; };",
							  true);
	// accessing private variable from other context is bad :)
	check_task.do_build_test("x0 = task { pre = {{ other::myPrivatevariable == 0.0 }};"
													      " post = {}; };",
							  false);
	// in our local context, it is ok
	check_task.do_build_test("x1 = task { pre = {{ first::myPrivatevariable == 0.0 }};"
													      " post = {}; };",
							 true);

	// calling unknow function is bad
	check_task.do_build_test("x2 = task { pre = {{ toto() }};"
													      " post = {}; };", false);

	// calling valid function with bad argument number is not really better
	check_task.do_build_test("x3 = task { pre = {{ square() }};"
													      " post = {}; };", false);

	// calling valid function with constant
	check_task.do_build_test("x4 = task { pre = {{ square(42.0) == 0.0}};"
													      " post = {}; };", true);

	// calling valid function with variable of good type is ok
	check_task.do_build_test("x5 = task { pre = {{ square(thresold) == 0.0}};"
													      " post = {}; };", true);

	// calling valid function with variable of bad type is bad
	check_task.do_build_test("x6 = task { pre = {{ square(j) }};"
													      " post = {}; };", false);

	// calling valid function with non-accessible variable is not really good
	check_task.do_build_test("x7 = task { pre = {{ square(other::myPrivatevariable) == 0.0}};"
													      " post = {}; };", false);

	// calling operator < with two variables on same kind is ok
	check_task.do_build_test("x8 = task { pre = {{ i < j }};"
													      " post = {}; };", true);
	
	// calling operator < with variables of different kind is definitivly forbidden
	check_task.do_build_test("x9 = task { pre = {{ i < 42.0 }};"
													      " post = {}; };", false);

	// calling operator < with result of a function is ok as long as the two types match
	check_task.do_build_test("x10 = task { pre = {{ square(thresold)  < 42.0 }};"
													      " post = {}; };", true);

	// same kind of rules for == and !=
	check_task.do_build_test("x11 = task { pre = {{ i == j }};"
													      " post = {}; };", true);
	
	check_task.do_build_test("x12 = task { pre = {{ i == 42.0 }};"
													      " post = {}; };", false);

	check_task.do_build_test("x13 = task { pre = {{ square(thresold) == 42.0 }};"
													      " post = {}; };", true);

	// for operator && and ||, we only accept boolean expression
	check_task.do_build_test("x14 = task { pre = {{ square(thresold)  &&  42.0 }};"
													      " post = {}; };", false);

	check_task.do_build_test("x15 = task { pre = {{ (i > j)  &&  (j > k) }};"
													      " post = {}; };", true);

	// for operator -, only accept int and double in entry (probably not correct)
	check_task.do_build_test("x16 = task { pre = {{ - (i < j) }};"
													      " post = {}; };", false);

	check_task.do_build_test("x17 = task { pre = {{ - square(42.0) == 8.0 }};"
													      " post = {}; };", true);


	// A condition must be a valid expression and returns a bool
	check_task.do_build_test("x18 = task { pre = {{ square(thresold)}};"
						      " post = {}; };", false);

	check_task.do_build_test("x19 = task { pre = {{ first::myPrivatevariable}};"
													      " post = {}; };", false);
}


