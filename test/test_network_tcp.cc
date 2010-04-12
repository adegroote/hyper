#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>

#include <network/server_tcp_impl.hh>
#include <network/client_tcp_impl.hh>

// A simple echo server

using namespace hyper::network;
using namespace hyper::network::tcp;

typedef message_types input_msg;
typedef message_types output_msg;
typedef boost::make_variant_over<input_msg>::type input_variant;
typedef boost::make_variant_over<input_msg>::type output_variant;

struct echo_visitor : public boost::static_visitor<output_variant>
{
	template <typename T>
	output_variant operator() (const T& t) const
	{
		return t;
	}
};

typedef server<input_msg, output_msg, echo_visitor>  echo_server;

typedef sync_client<output_msg> echo_client;

BOOST_AUTO_TEST_CASE ( network_tcp_async_test )
{
	echo_server s("127.0.0.1", "4242");
	boost::thread thr( boost::bind(& echo_server::run, &s));

	boost::asio::io_service io_s;
	echo_client c(io_s, "127.0.0.1", "4242");
	boost::thread thr1( boost::bind( & boost::asio::io_service::run, &io_s));

	request_name rn1, rn2;
	rn1.name = "one ability";
	c.sync_request(rn1, rn2);

	BOOST_CHECK(rn1.name == rn2.name);

	struct request_name_answer request_name_answer1, request_name_answer2;
	request_name_answer1.success = true;
	request_name_answer1.name = "myAbility";
	request_name_answer1.endpoint = boost::asio::ip::tcp::endpoint(
										boost::asio::ip::address::from_string("127.0.0.1"), 4242);

	c.sync_request(request_name_answer1, request_name_answer2);

	BOOST_CHECK(request_name_answer1.name == request_name_answer2.name);
	BOOST_CHECK(request_name_answer1.success == request_name_answer2.success);
	BOOST_CHECK(request_name_answer1.endpoint == request_name_answer2.endpoint);

	struct register_name register_name1, register_name2;
	register_name1.name = "myAbility";
	register_name1.endpoint = boost::asio::ip::tcp::endpoint(
							  boost::asio::ip::address::from_string("227.0.52.1"), 4242);

	c.sync_request(register_name1, register_name2);

	BOOST_CHECK(register_name1.name == register_name2.name);
	BOOST_CHECK(register_name1.endpoint == register_name2.endpoint);

	struct register_name_answer register_name_answer1, register_name_answer2;
	register_name_answer1.name = "otherAbility";
	register_name_answer1.success = false;

	c.sync_request(register_name_answer1, register_name_answer2);

	BOOST_CHECK(register_name_answer1.name == register_name_answer2.name);
	BOOST_CHECK(register_name_answer1.success == register_name_answer2.success);

	/* It is an echo server, so expecting a struct in output different than the
	 * one in input must lead to an exception boost::system::system_error
	 */
	BOOST_CHECK_THROW(c.sync_request(register_name1, rn2), boost::system::system_error);

	s.stop();
	c.close();
	thr.join();
	thr1.join();
}
