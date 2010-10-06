#include <hyperConfig.hh>

#include <compiler/symbols.hh>
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE ( compiler_symbols_test )
{
	using namespace hyper::compiler;

	typeList tlist;
	symbolList slist(tlist);

	tlist.add("double", doubleType);
	tlist.add("void", noType);
	tlist.add("int", intType);

	std::pair < bool, symbolList::symbolAddError > p;
	p = slist.add("toto", "double");
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second == symbolList::noError);

	p = slist.add("titi", "int");
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second == symbolList::noError);

	p = slist.add("42", "noType");
	BOOST_CHECK(p.first == false);
	BOOST_CHECK(p.second == symbolList::unknowType);

	p = slist.add("toto", "int");
	BOOST_CHECK(p.first == false);
	BOOST_CHECK(p.second == symbolList::alreadyExist);

	std::pair < bool, symbol > s;
	s = slist.get("toto");
	BOOST_CHECK( s.first == true);
	BOOST_CHECK( s.second.name == "toto");
	BOOST_CHECK( s.second.t == 0);

	s = slist.get("77");
	BOOST_CHECK ( s.first == false);
}
