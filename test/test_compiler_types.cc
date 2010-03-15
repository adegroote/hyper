#include <compiler/types.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( Compiler_types_test)
{
	using namespace hyper::compiler;

	typeList list;

	typeList::add_result p;

	// typeList::add check
	p = list.add("double", doubleType);
	BOOST_CHECK(p.get<0>() == true);
	BOOST_CHECK(p.get<1>() == 0);

	p = list.add("int", intType);
	BOOST_CHECK(p.get<0>() == true);
	BOOST_CHECK(p.get<1>() == 1);

	p = list.add("string", stringType);
	BOOST_CHECK(p.get<0>() == true);
	BOOST_CHECK(p.get<1>() == 2);

	p = list.add("int", stringType);
	BOOST_CHECK(p.get<0>() == false);
	BOOST_CHECK(p.get<1>() == 1);

	std::pair < bool, typeList::typeId > p1;
	// typeList::getId check
	p1 = list.getId("int");
	BOOST_CHECK(p1.first == true);
	BOOST_CHECK(p1.second == 1);

	p1 = list.getId("long");
	BOOST_CHECK(p1.first == false);
	// don't care about p.second
	
	type t = list.get(1);
	BOOST_CHECK(t.name == "int");
	BOOST_CHECK(t.t == intType);
}
