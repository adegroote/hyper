#include <compiler/utils.hh>
#include <boost/test/unit_test.hpp>

#include <functional>

struct different_from_1 {
	typedef int argument_type;
	typedef bool result_type;

	bool operator() (int i) const
	{
		return (i != 1);
	}
};

BOOST_AUTO_TEST_CASE ( compiler_utils_test )
{
	std::vector<int> v, res, res2;
	v.push_back(-2);
	v.push_back(-1);
	v.push_back(0);
	v.push_back(1);
	v.push_back(2);

	hyper::compiler::copy_if(v.begin(), v.end(), std::back_inserter(res), 
							 std::bind2nd(std::less<int>(), 0));

	BOOST_CHECK(res.size() == 2);
	BOOST_CHECK(res[0] == -2);
	BOOST_CHECK(res[1] == -1);

	hyper::compiler::copy_if(v.begin(), v.end(), std::back_inserter(res2),
							different_from_1());

	BOOST_CHECK(res2.size() == 4);
	BOOST_CHECK(res2[0] == -2);
	BOOST_CHECK(res2[1] == -1);
	BOOST_CHECK(res2[2] == 0);
	BOOST_CHECK(res2[3] == 2);

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
