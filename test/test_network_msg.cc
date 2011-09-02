#include <network/msg.hh>
#include <boost/test/unit_test.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

template <typename T>
void try_archive_interface(const T& src, T& dest)
{
	std::string tmp;
	{
		std::ostringstream archive_stream;
		boost::archive::binary_oarchive archive(archive_stream);
		archive << src;
		tmp = archive_stream.str();
	}

	{ 
		std::istringstream archive_stream(tmp);
		boost::archive::binary_iarchive archive(archive_stream);
		archive >> dest;
	}
}

BOOST_AUTO_TEST_CASE ( network_msg_test )
{

	using namespace hyper::network;
	using namespace hyper::logic;

	request_name request_name1, request_name2;
	request_name1.name = "myAbility";

	try_archive_interface(request_name1, request_name2);

	BOOST_CHECK(request_name1.name == request_name2.name);

	request_name_answer request_name_answer1, request_name_answer2;
	request_name_answer1.success = true;
	request_name_answer1.name = "myAbility";
	request_name_answer1.endpoints.push_back(boost::asio::ip::tcp::endpoint(
										boost::asio::ip::address::from_string("127.0.0.1"), 4242));

	try_archive_interface(request_name_answer1, request_name_answer2);

	BOOST_CHECK(request_name_answer1.name == request_name_answer2.name);
	BOOST_CHECK(request_name_answer1.success == request_name_answer2.success);
	BOOST_CHECK(request_name_answer1.endpoints == request_name_answer2.endpoints);


	register_name register_name1, register_name2;
	register_name1.name = "myAbility";
	register_name1.endpoints.push_back(boost::asio::ip::tcp::endpoint(
							  boost::asio::ip::address::from_string("227.0.52.1"), 4242));

	try_archive_interface(register_name1, register_name2);

	BOOST_CHECK(register_name1.name == register_name2.name);
	BOOST_CHECK(register_name1.endpoints == register_name2.endpoints);

	register_name_answer register_name_answer1, register_name_answer2;
	register_name_answer1.name = "otherAbility";
	register_name_answer1.success = false;

	try_archive_interface(register_name_answer1, register_name_answer2);

	BOOST_CHECK(register_name_answer1.name == register_name_answer2.name);
	BOOST_CHECK(register_name_answer1.success == register_name_answer2.success);

	register_name_answer1.success = true;

	try_archive_interface(register_name_answer1, register_name_answer2);

	BOOST_CHECK(register_name_answer1.name == register_name_answer2.name);
	BOOST_CHECK(register_name_answer1.success == register_name_answer2.success);

	request_constraint2 ctr1, ctr2;
	function_call func("add_double", expression(Constant<double>(2.0)),
								     function_call("distance", std::string("goal"), std::string("current")));
	ctr1.id = 42;
	ctr1.src = "pipo";
	ctr1.constraint = func;
	ctr1.repeat = true;
	ctr1.unify_list.push_back(std::make_pair(std::string("pipo"), std::string("toto")));
	ctr1.unify_list.push_back(std::make_pair(std::string("pipo2"), Constant<double>(3.2)));

	try_archive_interface(ctr1, ctr2);

	BOOST_CHECK(ctr1.id == ctr2.id);
	BOOST_CHECK(ctr1.src == ctr2.src);
	BOOST_CHECK(ctr1.repeat == ctr2.repeat);
	// don't compare constraint with ==, because it use internal representation
	// id. The id is not safe accross network, so will be computed manually on the other side.
	// compare their string representation ...
	std::ostringstream oss1, oss2;
	oss1 << ctr1.constraint;
	oss2 << ctr2.constraint;
	BOOST_CHECK(oss1.str() == oss2.str());
}
