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

	s.stop();
	c.close();
	thr.join();
	thr1.join();
}
