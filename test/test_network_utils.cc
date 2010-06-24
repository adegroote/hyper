#include <network/utils.hh>
#include <boost/test/unit_test.hpp>

#include <boost/mpl/equal.hpp>

BOOST_AUTO_TEST_CASE ( network_utils_test )
{
	typedef boost::mpl::vector<int, int, double> vTypes;
	typedef boost::tuple<int, int, double> tuple_res;
	typedef hyper::network::to_tuple<vTypes>::type pipo;

	BOOST_MPL_ASSERT((boost::mpl::equal<pipo, tuple_res>));
}
