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
}
