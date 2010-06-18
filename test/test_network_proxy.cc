#include <network/proxy.hh>
#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>

namespace {
	using namespace hyper::network;

	struct test { int x, y, z;  std::string s;};

	struct proxy_server 
	{
		typedef tcp::server<proxy_input_msg, proxy_output_msg, proxy_visitor>
			tcp_impl;

		proxy_serializer s;
		tcp_impl prox;

		proxy_server(const std::string& addr, const std::string& port, boost::asio::io_service& io_s):
			s(), prox(addr, port, proxy_visitor(s), io_s) {}

		void stop()
		{
			prox.stop();
		}

		proxy_serializer& serializer() { return s; };
	};
}

BOOST_AUTO_TEST_CASE ( network_proxy_test )
{
	using namespace hyper::network;

	// Local proxy interface test
	{
		int i = 0;
		proxy<int> proxy_i(i);

		BOOST_CHECK(proxy_i() == 0);

		i = 1;

		BOOST_CHECK(proxy_i() == 1);
	}


	test t; t.x = 8;

	// Proxy serializer test
	{
		proxy_serializer s;
		int i;
		BOOST_CHECK(s.register_variable("x", t.x) == true);
		BOOST_CHECK(s.register_variable("y", t.y) == true);
		BOOST_CHECK(s.register_variable("z", t.z) == true);
		BOOST_CHECK(s.register_variable("x", i) == false);
	
		std::pair<bool, std::string> p, p1;
	
		p = s.eval("toto");
		BOOST_CHECK(p.first == false);
		p = s.eval("x");
		BOOST_CHECK(p.first == true);
		t.x = 9;
		p1 = s.eval("x");
		BOOST_CHECK(p.first == true);
		BOOST_CHECK(p.second != p1.second);
	}

	// Proxy visitor test
	{
		boost::asio::io_service io_s;
		test t;
		proxy_server s("127.0.0.1", "5000", io_s);
		boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_s));

		proxy_serializer& serialize = s.serializer();
		serialize.register_variable("x", t.x);
		serialize.register_variable("y", t.y);
		serialize.register_variable("mystring", t.s);

		typedef tcp::client<proxy_output_msg> proxy_client;
		proxy_client proxy_c(io_s);
		proxy_c.connect("127.0.0.1", "5000");

		request_variable_value value_request;
		variable_value value;

		value_request.var_name = "toto";
		proxy_c.request(value_request, value);

		BOOST_CHECK(value.var_name == "toto");
		BOOST_CHECK(value.success == false);

		t.x = 8;
		value_request.var_name = "x";
		proxy_c.request(value_request, value);
		BOOST_CHECK(value.var_name == "x");
		BOOST_CHECK(value.success == true);
		boost::optional<int> x_value = deserialize_value<int> ( value.value );
		BOOST_CHECK(x_value);
		BOOST_CHECK(*x_value == 8);

		t.x = 9;
		proxy_c.request(value_request, value);
		BOOST_CHECK(value.var_name == "x");
		BOOST_CHECK(value.success == true);
		x_value = deserialize_value<int> ( value.value );
		BOOST_CHECK(x_value);
		BOOST_CHECK(*x_value == 9);

		t.s = "example";
		value_request.var_name = "mystring";
		proxy_c.request(value_request, value);
		BOOST_CHECK(value.var_name == "mystring");
		BOOST_CHECK(value.success == true);
		boost::optional<std::string> s_value = deserialize_value<std::string> ( value.value );
		BOOST_CHECK(s_value);
		BOOST_CHECK(*s_value == "example");

		boost::optional<ping> s_bug_value = deserialize_value<ping> (value.value);
		BOOST_CHECK(!s_bug_value);

		s.stop();
		thr.join();
	}
	
}
