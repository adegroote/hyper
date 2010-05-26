#include <compiler/ability.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( compiler_ability_test )
{
	using namespace hyper::compiler;

	typeList tlist;
	tlist.add("int", intType);

	symbolList private_symbols(tlist);
	symbolList control_symbols(tlist);
	symbolList read_symbols(tlist);

	private_symbols.add("p1", "int");
	private_symbols.add("p2", "int");

	control_symbols.add("c1", "int");
	control_symbols.add("c2", "int");

	read_symbols.add("r1", "int");
	read_symbols.add("r2", "int");

	ability test("test", control_symbols, read_symbols, private_symbols);

	std::pair<bool, symbolACL> p;
	p = test.get_symbol("p1");
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second.acl == PRIVATE);
	BOOST_CHECK(p.second.s.name == "p1");

	p = test.get_symbol("p3");
	BOOST_CHECK(p.first == false);

	p = test.get_symbol("c1");
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second.acl == CONTROLABLE);
	BOOST_CHECK(p.second.s.name == "c1");

	p = test.get_symbol("r1");
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second.acl == READABLE);
	BOOST_CHECK(p.second.s.name == "r1");

}
