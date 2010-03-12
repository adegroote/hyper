#include <compiler/parser.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( compiler_parser_test )
{
	using namespace hyper::compiler;

	parser P;
	bool r;
	r = P.parse("int toto ;");
	r = P.parse("bool titi;");
	r = P.parse("root root;");
	r = P.parse("int f(double, double);");
	r = P.parse("double g(int , string);");
	r = P.parse("root k(int, string, toto)");
}
