#include <compiler/types.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( Compiler_types_test)
{
	using namespace hyper::compiler;

	typeList list;

	std::pair < bool, typeList::typeId > p;

	// typeList::add check
	p = list.add("double", doubleType);
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second == 0);

	p = list.add("int", intType);
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second == 1);

	p = list.add("string", stringType);
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second == 2);

	p = list.add("int", stringType);
	BOOST_CHECK(p.first == false);
	BOOST_CHECK(p.second == 1);

	// typeList::getId check
	p = list.getId("int");
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second == 1);

	p = list.getId("long");
	BOOST_CHECK(p.first == false);
	// don't care about p.second
	
	type t = list.get(1);
	BOOST_CHECK(t.name == "int");
	BOOST_CHECK(t.t == intType);
}
