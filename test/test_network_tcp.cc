#include <boost/test/unit_test.hpp>

#include <network/server_tcp_impl.hh>
#include <network/client_tcp_impl.hh>

#include <boost/thread.hpp>

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

typedef client<output_msg> echo_client;

template <typename T>
void are_equal(const T& a, const T&b) {};

template <>
void are_equal<request_name> (const request_name& a, const request_name& b)
{
	BOOST_CHECK(a.name == b.name);
}

template <>
void are_equal<request_name_answer> (const request_name_answer& a, 
									 const request_name_answer& b)
{
	BOOST_CHECK(a.name == b.name);
	BOOST_CHECK(a.success == b.success);
	BOOST_CHECK(a.endpoint == b.endpoint);
}

template <>
void are_equal<register_name> (const register_name& a,
							   const register_name& b)
{
	BOOST_CHECK(a.name == b.name);
}

template <>
void are_equal<register_name_answer> (const register_name_answer&a,
									  const register_name_answer&b)
{
	BOOST_CHECK(a.name == b.name);
	BOOST_CHECK(a.success == b.success);
	if (a.success) {
		BOOST_CHECK(a.endpoint == b.endpoint);
	}
}

template <typename OutputM>
struct test_async_client
{
	client<OutputM> c;
	
	explicit test_async_client(boost::asio::io_service &io_s,
			const std::string &addr, const std::string& port):
		c(io_s)
	{
		c.async_connect(addr, port,
				boost::bind(&test_async_client::handle_connect, 
							this,
							boost::asio::placeholders::error));
		r1.name = "my ability";
		rn1.name = "myAbility";
	}

	void handle_connect(const boost::system::error_code& e)
	{
		if (e)
		{
			std::cerr << "Can't connect : " << std::endl;
		} else {

			c.async_request(r1, r2,
						   boost::bind(&test_async_client::handle_request_name_test,
									   this,
									   boost::asio::placeholders::error));
		}
	}

	void handle_request_name_test(const boost::system::error_code& e)
	{
		if (e)
		{
			std::cerr << "Error processing msg" << std::endl;
		} else {
			are_equal(r1, r2);
			c.async_request(rn1, rn2, 
					boost::bind(&test_async_client::handle_request_register_name,
								this,
								boost::asio::placeholders::error));
		}
	}

	void handle_request_register_name(const boost::system::error_code& e)
	{
		if (e)
		{
			std::cerr << "Error processing msg" << std::endl;
		}
		else 
		{
			are_equal(rn1, rn2);
		}
	}

	request_name r1, r2;
	register_name rn1, rn2;

};

BOOST_AUTO_TEST_CASE ( network_tcp_async_test )
{
	boost::asio::io_service io_s;
	echo_server s("127.0.0.1", "4242", echo_visitor(), io_s);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_s));

	echo_client c(io_s);

	request_name rn1, rn2;
	rn1.name = "one ability";

	/* Exception launched if no connection */
	BOOST_CHECK_THROW(c.request(rn1, rn2), boost::system::system_error);

	c.connect("127.0.0.1", "4242");
	c.request(rn1, rn2);
	are_equal(rn1, rn2);

	struct request_name_answer request_name_answer1, request_name_answer2;
	request_name_answer1.success = true;
	request_name_answer1.name = "myAbility";
	request_name_answer1.endpoint = boost::asio::ip::tcp::endpoint(
										boost::asio::ip::address::from_string("127.0.0.1"), 4242);

	c.request(request_name_answer1, request_name_answer2);
	are_equal(request_name_answer1, request_name_answer2);

	struct register_name register_name1, register_name2;
	register_name1.name = "myAbility";

	c.request(register_name1, register_name2);
	are_equal(register_name1, register_name2);


	struct register_name_answer register_name_answer1, register_name_answer2;
	register_name_answer1.name = "otherAbility";
	register_name_answer1.success = false;

	c.request(register_name_answer1, register_name_answer2);
	are_equal(register_name_answer1, register_name_answer2);

	register_name_answer1.success = true;
	register_name_answer1.endpoint = boost::asio::ip::tcp::endpoint(
							  boost::asio::ip::address::from_string("227.0.52.1"), 4242);

	c.request(register_name_answer1, register_name_answer2);
	are_equal(register_name_answer1, register_name_answer2);

	/* It is an echo server, so expecting a struct in output different than the
	 * one in input must lead to an exception boost::system::system_error
	 */
	BOOST_CHECK_THROW(c.request(register_name1, rn2), boost::system::system_error);

	c.close();
	/* Exception launched if the socket has been closed */
	BOOST_CHECK_THROW(c.request(rn1, rn2), boost::system::system_error);

	boost::asio::io_service io_s2;
	test_async_client<output_msg> test(io_s2, "127.0.0.1", "4242");
	io_s2.run();

	s.stop();
	c.close();
	thr.join();
}
