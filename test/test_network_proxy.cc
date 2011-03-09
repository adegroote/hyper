#include <network/proxy.hh>
#include <boost/test/unit_test.hpp>

#include <boost/assign/list_of.hpp>
#include <boost/thread.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/equal.hpp>

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

	struct false_resolv
	{
		false_resolv() {};

		template <typename Handler>
		void async_resolve(name_resolve & solv, Handler handler)
		{
			solv.rna.endpoints.clear();
			solv.rna.endpoints.push_back(boost::asio::ip::tcp::endpoint(
					  boost::asio::ip::address::from_string("127.0.0.1"), 5000));
			handler(boost::system::error_code());
		}
	};

	struct test_proxy {
		false_resolv resolver;
		remote_proxy<int, false_resolv> r_proxy;
		test & t;
		

		test_proxy(boost::asio::io_service& io_s, test& t_) :
			r_proxy(io_s, "test", "x" , resolver), t(t_) {}

		void handle_second_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			const boost::optional<int>& value = r_proxy();
			BOOST_CHECK(value);
			BOOST_CHECK(*value == 14);
		}

		void handle_first_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			const remote_proxy<int,false_resolv>::type & value = r_proxy();
			BOOST_CHECK(value);
			BOOST_CHECK(*value == 12);

			t.x = 14;
			r_proxy.async_get(
					boost::bind(&test_proxy::handle_second_test, 
								this, boost::asio::placeholders::error));
		}

		void test_async()
		{
			t.x = 12;
			r_proxy.async_get(
					boost::bind(&test_proxy::handle_first_test, 
								this, boost::asio::placeholders::error));
		}
	};

	struct test_proxy_type_erasure
	{
		false_resolv resolver;
		remote_proxy_base* r_proxy;
		test & t;
		

		test_proxy_type_erasure(boost::asio::io_service& io_s, test& t_) : t(t_) 
		{
			r_proxy = new remote_proxy<int, false_resolv>(io_s, "test", "x", resolver);
		}

		~test_proxy_type_erasure()
		{
			delete r_proxy;
		}

		void handle_second_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			boost::any res = r_proxy->extract_result();
			boost::optional<int> value = boost::any_cast<boost::optional<int> > (res);
			BOOST_CHECK(value);
			BOOST_CHECK(*value == 14);

			std::cerr << "test_proxy_type_erasure::second_test" << std::endl;
		}

		void handle_first_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			boost::any res = r_proxy->extract_result();
			boost::optional<int> value = boost::any_cast<boost::optional<int> > (res);
			BOOST_CHECK(value);
			BOOST_CHECK(*value == 12);

			std::cerr << "test_proxy_type_erasure::first_test" << std::endl;

			t.x = 14;
			r_proxy->async_get_result(
					boost::bind(&test_proxy_type_erasure::handle_second_test, 
								this, boost::asio::placeholders::error));
		}

		void test_async()
		{
			t.x = 12;
			r_proxy->async_get_result(
					boost::bind(&test_proxy_type_erasure::handle_first_test, 
								this, boost::asio::placeholders::error));
		}
	};

	struct test_proxy_error {
		false_resolv resolver;
		remote_proxy<int, false_resolv> r_proxy;
		test & t;
		

		test_proxy_error(boost::asio::io_service& io_s, test& t_) :
			r_proxy(io_s, "test", "pipo" , resolver), t(t_) {}

		void handle_first_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e); // no system error
			// XXX must be improved to return a useful error code
			const boost::optional<int>& value = r_proxy();
			BOOST_CHECK(!value); // but value is "undefined" 
		}

		void test_async()
		{
			r_proxy.async_get(
					boost::bind(&test_proxy_error::handle_first_test, 
								this, boost::asio::placeholders::error));
		}
	};

	struct test_remote_proxy_n {
		typedef boost::mpl::vector<int, int, std::string> local_types;
		typedef remote_proxy_n<local_types, false_resolv> r_proxy_n;

		false_resolv resolv_;
		r_proxy_n r_proxy;

		test_remote_proxy_n(boost::asio::io_service& io_s) :
			r_proxy(io_s,
					boost::assign::list_of<std::pair<std::string, std::string> >
						("test", "x")("test","y")("test", "mystring"),
					resolv_)
			{}

		void handle_test(const boost::system::error_code& e)
		{
			const boost::optional<int> &x = r_proxy.at_c<0>();
			const boost::optional<int> &y = r_proxy.at_c<1>();
			const boost::optional<std::string> &s = r_proxy.at_c<2>();

			std::cout << "we really enter here" << std::endl;

			BOOST_CHECK(!e);
			BOOST_CHECK(x);
			BOOST_CHECK(y);
			BOOST_CHECK(s);

			BOOST_CHECK(r_proxy.is_successful());

			BOOST_CHECK(*x == 42);
			BOOST_CHECK(*y == 43);
			BOOST_CHECK(*s == "wonderland");
		}

		void test_async()
		{
			r_proxy.async_get(
					boost::bind(
						&test_remote_proxy_n::handle_test,
						this, boost::asio::placeholders::error));
		}
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

	// local proxy test
	{
		local_proxy l;
		int i;
		BOOST_CHECK(l.register_variable("x", t.x) == true);
		BOOST_CHECK(l.register_variable("y", t.y) == true);
		BOOST_CHECK(l.register_variable("z", t.z) == true);
		BOOST_CHECK(l.register_variable("x", i) == false);

		t.x = 99;
		boost::optional<int> x_value;
		x_value = l.eval<int>("x");
		BOOST_CHECK(x_value);
		BOOST_CHECK(*x_value = 99);

		t.x = 101;
		x_value = l.eval<int>("x");
		BOOST_CHECK(x_value);
		BOOST_CHECK(*x_value = 101);

		x_value = l.eval<int>("i");
		BOOST_CHECK(! x_value);

		boost::optional<double> y_value;
		y_value = l.eval<double>("y");
		BOOST_CHECK(! y_value); // bad type
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

		boost::asio::io_service io2_s;
		test_proxy_error test_p_error(io2_s, t);
		test_p_error.test_async();
		io2_s.run();
		io2_s.reset();

		test_proxy test_p(io2_s, t);
		test_p.test_async();
		io2_s.run();
		io2_s.reset();

		test_proxy_type_erasure test_p_erasure(io2_s, t);
		test_p_erasure.test_async();
		io2_s.run();
		io2_s.reset();

		false_resolv resolv_;
		t.x = 42;
		t.y = 43;
		t.s = "wonderland";
		test_remote_proxy_n r_proxy_n(io2_s);
		r_proxy_n.test_async();
		io2_s.run();
		io2_s.reset();

		t.x = 92;
		remote_proxy_sync<int, false_resolv> sync_proxy(io2_s, "test", "x", resolv_);
		boost::optional<int> res_sync_proxy = sync_proxy.get(boost::posix_time::seconds(1));
		BOOST_CHECK(res_sync_proxy);
		BOOST_CHECK(*res_sync_proxy == 92);
		io2_s.reset();


		// XXX Need to test timeout version ...

		s.stop();
		thr.join();
	}
}
