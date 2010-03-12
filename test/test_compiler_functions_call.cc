#include <compiler/functions_call.hh>
#include <boost/test/unit_test.hpp>

#include <boost/assign/list_of.hpp>

#include <iostream>

BOOST_AUTO_TEST_CASE ( compiler_functions_call_test )
{
	using namespace hyper::compiler;
	using namespace boost::assign;

	typeList list;
	functionDefList flist(list);
	symbolList slist(list);
	functionCallEnv env(list, slist, flist);

	// prepare the environment
	
	list.add("void", noType);
	list.add("double", doubleType);
	list.add("int", intType);
	list.add("string", stringType);

	slist.add("myDouble", "double");
	slist.add("myString1", "string");
	slist.add("myString2", "string");
	slist.add("myInt", "int");

	std::vector < std::string > v;

	v = list_of("double");
	flist.add("square", "double", v);

	v = list_of("double")("double")("double")("double");
	flist.add("distance", "double", v);


	v = list_of("string");
	flist.add("print", "void", v);

	v = list_of("double");
	flist.add("convert", "string", v);

	function_node_string s;
	s = list_of("square")("myDouble");
	functionCall f  = env.make(s);
	BOOST_CHECK(f.toString() == "square(myDouble)");

	s = list_of("distance")("myDouble")("myDouble")("myDouble")("myDouble");
	functionCall f1 = env.make(s);
	BOOST_CHECK(f1.toString() == "distance(myDouble, myDouble, myDouble, myDouble)");

	s = list_of("square")("myString1");
	BOOST_CHECK_THROW(env.make(s), functionCallEnvExceptionInvalidArgsType);

	s = list_of("square")("myDouble")("myDouble");
	BOOST_CHECK_THROW(env.make(s), functionCallEnvExceptionBadArity);

	s = list_of("unknowFunction")("myDouble");
	BOOST_CHECK_THROW(env.make(s), functionCallEnvExceptionExpectedFunctionName);

	s = list_of("square")("newDouble");
	BOOST_CHECK_THROW(env.make(s), functionCallEnvExpectedSymbol);

	function_node_string s1;
	s1 = list_of("convert")("myDouble");
	s.clear();
	s.push_back("print");
	s.push_back(s1);
	functionCall f2 = env.make(s);
	BOOST_CHECK(f2.toString() == "print(convert(myDouble))");

	s1 = list_of("square")("myDouble");
	s.clear();
	s.push_back("print");
	s.push_back(s1);
	BOOST_CHECK_THROW(env.make(s), functionCallEnvExceptionInvalidReturnType);
}
