#include <network/client_tcp_impl.hh>
#include <network/nameserver.hh>

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

struct test_async_name
{
	hyper::network::name_client & client_;
	hyper::network::name_resolve r;


	test_async_name(hyper::network::name_client& client) : client_(client) {}

	void handle_first_test(const boost::system::error_code &e)
	{
		std::cout << "handling test_async_name::first_test_async" << std::endl;
		std::vector<boost::asio::ip::tcp::endpoint> endpoints;
		endpoints.push_back(boost::asio::ip::tcp::endpoint(
							boost::asio::ip::address_v4::any(), 4247));
		BOOST_CHECK(!e);
		BOOST_CHECK(r.endpoints() ==  endpoints);
	}

	void async_test() 
	{
		r.name("tata");
		client_.async_resolve(r,
				boost::bind(&test_async_name::handle_first_test, 
							this, 
							boost::asio::placeholders::error));
	}
};

BOOST_AUTO_TEST_CASE ( namesever_test )
{
	using namespace hyper::network;

	typedef boost::asio::ip::tcp::endpoint endpoint_type;
	typedef std::vector<endpoint_type> endpoint_vect;
	// testing map_addr (however, not the locking)
	
	{
	ns::map_addr map;

	std::pair<bool, ns::addr_storage> p;
	BOOST_CHECK( map.isIn("toto") == false );
	BOOST_CHECK ( map.remove("toto") == false );

	p = map.get("toto");
	BOOST_CHECK (p.first == false);


	endpoint_type endpoint1_ =  boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address::from_string("127.0.0.1"), 4242);
	endpoint_type endpoint2_ =  boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address::from_string("127.0.0.1"), 4343);
	endpoint_vect endpoint1; endpoint1.push_back(endpoint1_);
	endpoint_vect endpoint2; endpoint1.push_back(endpoint2_);

	ns::addr_storage addr;
	addr.tcp_endpoints = endpoint1;

	BOOST_CHECK ( map.add("toto", addr) == true);
	BOOST_CHECK ( map.isIn("toto") == true);
	BOOST_CHECK ( map.isIn("titi") == false);
	p = map.get("toto");
	BOOST_CHECK ( p.first == true );
	BOOST_CHECK ( p.second.tcp_endpoints ==  endpoint1);
	p = map.get("titi");
	BOOST_CHECK (p.first == false);

	addr.tcp_endpoints = endpoint2;
	BOOST_CHECK (map.add("toto", addr) == false);
	BOOST_CHECK ( map.isIn("toto") == true);
	p = map.get("toto");
	BOOST_CHECK ( p.first == true );
	BOOST_CHECK ( p.second.tcp_endpoints ==  endpoint1);

	BOOST_CHECK (map.remove("toto") == true);
	BOOST_CHECK (map.isIn("toto") == false);
	p = map.get("toto");
	BOOST_CHECK (p.first == false);
	}

	// testing nameserver
	boost::asio::io_service io_s;
	name_server s("127.0.0.1", "4242", io_s, false);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_s));
	typedef tcp::client<ns::output_msg> ns_client;

	endpoint_type endpointx =  boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address_v4::any(), 4243);
	endpoint_type endpointx_ =  boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address_v4::any(), 4245);
	endpoint_type endpointx__ =  boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address_v4::any(), 4247);

	endpoint_vect endpoint, endpoint_, endpoint__;
	endpoint.push_back(endpointx);
	endpoint_.push_back(endpointx_);
	endpoint__.push_back(endpointx__);

	ns_client c(io_s);

	c.connect("127.0.0.1", "4242");
	request_name rn;
	request_name_answer rna;

	rn.name = "pipo";
	c.request(rn, rna);

	BOOST_CHECK(rna.name == rn.name);
	BOOST_CHECK(rna.success == false);

	register_name re;
	register_name_answer rea;

	re.name = "pipo";
	re.endpoints = endpoint;

	c.request(re, rea);

	BOOST_CHECK(rea.name == re.name);
	BOOST_CHECK(rea.success == true);

	c.request(rn, rna);
	BOOST_CHECK(rna.name == rn.name);
	BOOST_CHECK(rna.success == true);
	BOOST_CHECK(rna.endpoints == endpoint);

	// trying to register again the same name fails
	re.name = "pipo";

	c.request(re, rea);
	re.endpoints = endpoint_;

	BOOST_CHECK(rea.name == re.name);
	BOOST_CHECK(rea.success == false);

	// but we keep the previous addr
	c.request(rn, rna);
	BOOST_CHECK(rna.name == rn.name);
	BOOST_CHECK(rna.success == true);
	BOOST_CHECK(rna.endpoints == endpoint);

	// remove an entry
	s.remove_entry(re.name);

	// entry is not more here
	c.request(rn, rna);

	BOOST_CHECK(rna.name == rn.name);
	BOOST_CHECK(rna.success == false);

	// adding it again
	c.request(re, rea);
	BOOST_CHECK(rea.name == re.name);
	BOOST_CHECK(rea.success == true);

	c.request(rn, rna);
	BOOST_CHECK(rna.name == rn.name);
	BOOST_CHECK(rna.success == true);
	BOOST_CHECK(rna.endpoints == endpoint_);

	// test for name_client
	BOOST_CHECK_THROW(name_client(io_s, "localhost", "5000"), boost::system::system_error);

	name_client name_client(io_s, "localhost", "4242");
	std::pair<bool, std::vector<boost::asio::ip::tcp::endpoint> > p;

	p = name_client.sync_resolve("tata");

	BOOST_CHECK(p.first == false);

	p = name_client.sync_resolve("pipo");

	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second == endpoint_);

	bool res  = name_client.register_name("pipo", endpoint__);

	BOOST_CHECK(res == false);

	res  = name_client.register_name("tata", endpoint__);

	BOOST_CHECK(res == true);

	p = name_client.sync_resolve("tata");
	BOOST_CHECK(p.first == true);
	BOOST_CHECK(p.second == endpoint__);

	boost::asio::io_service ios2;
	hyper::network::name_client nc2(ios2, "localhost", "4242");
	test_async_name test_async(nc2);
	test_async.async_test();
	ios2.run();

	s.stop();
	thr.join();
}
