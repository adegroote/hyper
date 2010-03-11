#include <compiler/functions_def.hh>
#include <boost/test/unit_test.hpp>

#include <boost/assign/list_of.hpp>

BOOST_AUTO_TEST_CASE ( compiler_functions_def_test )
{
	using namespace hyper::compiler;
	using namespace boost::assign;

	typeList list;
	functionDefList flist(list);

	list.add("void", noType);
	list.add("int", intType);
	list.add("double", doubleType);

	std::vector < std::string > v = list_of("double");

	boost::tuple < bool, functionDefList::addErrorType, int > t;

	t = flist.add("square", "double", v);
	BOOST_CHECK(t.get<0>() == true);
	BOOST_CHECK(t.get<1>() == functionDefList::noError);
	

	v = list_of("double")("int")("int");
	t = flist.add("strange", "void", v);
	BOOST_CHECK(t.get<0>() == true);
	BOOST_CHECK(t.get<1>() == functionDefList::noError);

	v = list_of("int")("unknowStruct");
	t = flist.add("error", "void", v);
	BOOST_CHECK(t.get<0>() == false);
	BOOST_CHECK(t.get<1>() == functionDefList::unknowArgsType);
	BOOST_CHECK(t.get<2>() == 1);

	v = list_of("int");
	t = flist.add("error2", "unknowStruct", v);
	BOOST_CHECK(t.get<0>() == false);
	BOOST_CHECK(t.get<1>() == functionDefList::unknowReturnType);

	t = flist.add("strange", "void", v);
	BOOST_CHECK(t.get<0>() == false);
	BOOST_CHECK(t.get<1>() == functionDefList::alreadyExist);

	functionDef f;
	std::pair < bool, functionDef> p;
	p = flist.get("strange");
	BOOST_CHECK(p.first == true);
	f = p.second;
	BOOST_CHECK(f.name() == "strange");
	BOOST_CHECK(f.returnType() == 0);
	BOOST_CHECK(f.arity() == 3);
	BOOST_CHECK(f.argsType(0) == 2);
	BOOST_CHECK(f.argsType(1) == 1);
	BOOST_CHECK(f.argsType(2) == 1);

	p = flist.get("notdefined");
	BOOST_CHECK(p.first == false);
}
