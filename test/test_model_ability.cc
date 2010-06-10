#include <model/ability.hh>
#include <boost/test/unit_test.hpp>

struct ability : hyper::model::ability
{
	ability() : hyper::model::ability("pipo") {};

	double x, y, z;
};

BOOST_AUTO_TEST_CASE ( model_ability_test )
{
	// without nameserver, just throw a boost::system::system_error
	BOOST_CHECK_THROW(ability a, boost::system::system_error);
}
