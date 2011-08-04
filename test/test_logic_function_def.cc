#include <logic/function_def.hh>
#include <boost/test/unit_test.hpp>

#include <boost/optional/optional.hpp>

BOOST_AUTO_TEST_CASE ( logic_function_def_test)
{
	using namespace hyper::logic;

	funcDefList list;
	BOOST_CHECK(list.size() == 0);

	functionId id;
	id = list.add("equal", 2);
	BOOST_CHECK(id == 0);
	BOOST_CHECK(list.size() == 1);

	id = list.add("less", 2);
	BOOST_CHECK(id == 1);
	BOOST_CHECK(list.size() == 2);

	/* Adding again the same function just return an handle on previous
	 * definition */
	id = list.add("equal", 3);
	BOOST_CHECK(id == 0);
	BOOST_CHECK(list.size() == 2);

	function_def f;

	f = list.get(0);
	BOOST_CHECK(f.name == "equal");
	BOOST_CHECK(f.arity == 2);
	BOOST_CHECK(list.size() == 2);

	boost::optional<function_def> f_opt;
	f_opt = list.get("less");
	BOOST_CHECK(f_opt);
	BOOST_CHECK((*f_opt).name == "less");
	BOOST_CHECK((*f_opt).arity == 2);

	f_opt = list.get("toto");
	BOOST_CHECK(! f_opt );

	boost::optional<functionId> f_id;
	f_id = list.getId("less");
	BOOST_CHECK(f_id);
	BOOST_CHECK(*f_id == 1);

	f_id = list.getId("toto");
	BOOST_CHECK(! f_opt);
}
