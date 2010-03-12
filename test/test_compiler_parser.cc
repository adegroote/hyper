#include <compiler/parser.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( compiler_parser_test )
{
	using namespace hyper::compiler;

	parser P;
	bool r;
	r = P.parse("int toto ;");
	r = P.parse("bool titi;");
	r = P.parse("bool b1, _b2;");
	r = P.parse("root root;");
	r = P.parse("double myDoubleX, myDoubleY , myDoubleZ;");
	r = P.parse("int f(double, double);");
	r = P.parse("double g(int , string);");
	r = P.parse("root k(int, string, toto)");
	r = P.parse("double k(int, string, toto);");
	r = P.parse("double distance(double x1, double y1, double x2, double y2);");
}
