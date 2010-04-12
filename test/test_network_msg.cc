#include <network/msg.hh>
#include <network/socket_tcp_async_serialized.hh>
#include <boost/test/unit_test.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

template <typename T>
void try_archive_interface(const T& src, T& dest)
{
	std::string tmp;
	{
		std::ostringstream archive_stream;
		boost::archive::text_oarchive archive(archive_stream);
		archive << src;
		tmp = archive_stream.str();
	}

	{ 
		std::istringstream archive_stream(tmp);
		boost::archive::text_iarchive archive(archive_stream);
		archive >> dest;
	}
}

BOOST_AUTO_TEST_CASE ( network_msg_test )
{

	using namespace hyper::network;

	struct request_name request_name1, request_name2;
	request_name1.name = "myAbility";

	try_archive_interface(request_name1, request_name2);

	BOOST_CHECK(request_name1.name == request_name2.name);

	struct request_name_answer request_name_answer1, request_name_answer2;
	request_name_answer1.success = true;
	request_name_answer1.name = "myAbility";
	request_name_answer1.endpoint = boost::asio::ip::tcp::endpoint(
										boost::asio::ip::address::from_string("127.0.0.1"), 4242);

	try_archive_interface(request_name_answer1, request_name_answer2);

	BOOST_CHECK(request_name_answer1.name == request_name_answer2.name);
	BOOST_CHECK(request_name_answer1.success == request_name_answer2.success);
	BOOST_CHECK(request_name_answer1.endpoint == request_name_answer2.endpoint);


	struct register_name register_name1, register_name2;
	register_name1.name = "myAbility";
	register_name1.endpoint = boost::asio::ip::tcp::endpoint(
							  boost::asio::ip::address::from_string("227.0.52.1"), 4242);

	try_archive_interface(register_name1, register_name2);

	BOOST_CHECK(register_name1.name == register_name2.name);
	BOOST_CHECK(register_name1.endpoint == register_name2.endpoint);

	struct register_name_answer register_name_answer1, register_name_answer2;
	register_name_answer1.name = "otherAbility";
	register_name_answer1.success = false;

	try_archive_interface(register_name_answer1, register_name_answer2);

	BOOST_CHECK(register_name_answer1.name == register_name_answer2.name);
	BOOST_CHECK(register_name_answer1.success == register_name_answer2.success);
}
