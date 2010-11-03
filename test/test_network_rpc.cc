#include <network/rpc.hh>
#include <boost/test/unit_test.hpp>

#include <network/server_tcp_impl.hh>
#include <network/client_tcp_impl.hh>

#include <boost/thread.hpp>

// A simple echo server
using namespace hyper::network;
using namespace hyper::network::tcp;

namespace {
typedef message_types input_msg;
typedef message_types output_msg;
typedef boost::make_variant_over<input_msg>::type input_variant;
typedef boost::make_variant_over<input_msg>::type output_variant;

struct echo_visitor : public boost::static_visitor<output_variant>
{
	template <typename T>
	output_variant operator() (const T& t) const
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		return t;
	}
};

typedef server<input_msg, output_msg, echo_visitor>  echo_server;

typedef client<output_msg> echo_client;

template <typename T>
void are_equal(const T& a, const T&b) {}

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

struct false_resolv
{
	false_resolv() {};

	template <typename Handler>
	void async_resolve(name_resolve & solv, Handler handler)
	{
		solv.rna.endpoint = boost::asio::ip::tcp::endpoint(
				boost::asio::ip::address::from_string("127.0.0.1"), 4242);
		handler(boost::system::error_code());
	}
};

struct async_rpc_test
{
	false_resolv res;
	async_rpc<request_name, request_name, false_resolv> rpc_request;
	async_rpc<register_name, register_name, false_resolv> rpc_register;
	request_name rq1;
	register_name rn1;

	async_rpc_test(boost::asio::io_service &io_s) :
		rpc_request(io_s, "test", res),
		rpc_register(io_s, "test", res)
	{}

	void handle_second_test(const boost::system::error_code& e)
	{
		BOOST_CHECK(!e);
		const boost::optional<register_name>& value = rpc_register();
		BOOST_CHECK(value);
		are_equal(rn1, *value);
		std::cout << "Handling async_rpc_test::second_test" << std::endl;
	}

	void handle_first_test(const boost::system::error_code& e)
	{
		BOOST_CHECK(!e);
		const boost::optional<request_name>& value = rpc_request();
		BOOST_CHECK(value);
		are_equal(rq1, *value);
		std::cout << "Handling async_rpc_test::first_test" << std::endl;

		rn1.name = "myAbility";

		rpc_register.compute(rn1,
				boost::bind(&async_rpc_test::handle_second_test, 
					this, boost::asio::placeholders::error));
	}

	void do_test()
	{
		rq1.name = "myAbility";
		rpc_request.compute(rq1,
				boost::bind(&async_rpc_test::handle_first_test, 
					this, boost::asio::placeholders::error));
	}
};
}

BOOST_AUTO_TEST_CASE ( network_rpc_test )
{
	boost::asio::io_service io_s;
	echo_server s("127.0.0.1", "4242", echo_visitor(), io_s);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_s));

	boost::asio::io_service io2_s;
	async_rpc_test test(io2_s);
	test.do_test();
	io2_s.run();
	false_resolv resolv;

	io2_s.reset();
	{
	request_name rq1;
	rq1.name = "myOtherAbility";
	sync_rpc<request_name, request_name, false_resolv> rpc_request(io2_s, "test", resolv);
	const boost::optional<request_name>& res = rpc_request.compute(rq1, 
												boost::posix_time::milliseconds(100));
	BOOST_CHECK(res);
	are_equal(rq1, *res);
	std::cout << "rpc_request.compute first" << std::endl;
	}

	{
	boost::asio::io_service io3_s;
	request_name rq1;
	rq1.name = "myOtherAbility";
	sync_rpc<request_name, request_name, false_resolv> rpc_request(io3_s, "test", resolv);
	const boost::optional<request_name>& res = rpc_request.compute(rq1, 
												boost::posix_time::milliseconds(20));
	BOOST_CHECK(!res);
	std::cout << "rpc_request.compute second" << std::endl;
	}

	{
	io2_s.reset();
	sync_rpc<register_name_answer, register_name_answer, false_resolv>
	rpc_register_name_answer(io2_s, "test", resolv);
	register_name_answer rna1;
	rna1.success = true;
	rna1.endpoint = boost::asio::ip::tcp::endpoint(
							  boost::asio::ip::address::from_string("227.0.52.1"), 4242);
	const boost::optional<register_name_answer>& res3 = rpc_register_name_answer.compute(rna1, 
												boost::posix_time::milliseconds(100));
	BOOST_CHECK(res3);
	are_equal(rna1, *res3);
	}

	s.stop();
	thr.join();

}
