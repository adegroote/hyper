#include <compiler/functions_def.hh>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE ( compiler_functions_def_test )
{
	using namespace hyper::compiler;

	typeList list;
	std::pair < bool, typeList::typeId > p;

	p = list.add("void", noType);
	p = list.add("int", intType);
	p = list.add("double", doubleType);

	boost::array<std::string, 1> s = { "double" };
	functionDefImpl<1> square("square", "double", s, list);

	BOOST_CHECK (square.arity() == 1);
	BOOST_CHECK (square.returnType().name == "double");
	BOOST_CHECK (square.name() == "square");
	BOOST_CHECK (square.argsType(0).name == "double");

	boost::array<std::string, 3> s2 = { "double", "int", "int" };
	functionDefImpl<3> strange("strange", "void", s2, list);

	BOOST_CHECK (strange.arity() == 3);
	BOOST_CHECK (strange.returnType().name == "void");
	BOOST_CHECK (strange.name() == "strange");
	BOOST_CHECK (strange.argsType(0).name == "double");
	BOOST_CHECK (strange.argsType(1).name == "int");
	BOOST_CHECK (strange.argsType(2).name  == "int");

	boost::array< std::string, 0> s3 = {};
	functionDefImpl<0> empty("empty", "void", s3, list);
	BOOST_CHECK (empty.arity() == 0);
	BOOST_CHECK (empty.returnType().name == "void");
	BOOST_CHECK (empty.name() == "empty");

	BOOST_CHECK_THROW(functionDefImpl<0> strange2("strange2", "youdontknowthistype",
												  s3, list),
					  TypeFunctionDefException);

}
