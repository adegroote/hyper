#include <compiler/utils.hh>
#include <boost/test/unit_test.hpp>

#include <functional>

BOOST_AUTO_TEST_CASE ( compiler_utils_test )
{
	std::string replaced;
	std::string src1 = "a really long sentence but nothing to change";

	replaced = hyper::compiler::replace_by(src1, "@NAME@", "pos");
	BOOST_CHECK(replaced == src1);

	replaced = hyper::compiler::replace_by("my ability is @NAME@", "@NAME@", "pos");
	BOOST_CHECK(replaced == "my ability is pos");

	replaced = hyper::compiler::replace_by("@NAME@:@NAME@:@NAME@", "@NAME@", "pos");
	BOOST_CHECK(replaced == "pos:pos:pos");

	replaced = hyper::compiler::replace_by("@NAME@:@NAME@", "@NAME@", "@NAME@");
	BOOST_CHECK(replaced == "@NAME@:@NAME@");

	replaced = hyper::compiler::replace_by("@NAME@@", "@NAME@", "@NAME");
	BOOST_CHECK(replaced == "@NAME@");
}
