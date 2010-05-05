#include <network/client_tcp_impl.hh>
#include <network/nameserver.hh>

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

BOOST_AUTO_TEST_CASE ( namesever_test )
{
	using namespace hyper::network;

	// testing map_addr (however, not the locking)
	
	ns::map_addr map;

	std::pair<bool, ns::addr_storage> p;
	BOOST_CHECK( map.isIn("toto") == false );
	BOOST_CHECK ( map.remove("toto") == false );

	p = map.get("toto");
	BOOST_CHECK (p.first == false);

	boost::asio::ip::tcp::endpoint endpoint1 =  boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address::from_string("127.0.0.1"), 4242);
	boost::asio::ip::tcp::endpoint endpoint2 =  boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address::from_string("127.0.0.1"), 4343);
	ns::addr_storage addr;
	addr.tcp_endpoint = endpoint1;

	BOOST_CHECK ( map.add("toto", addr) == true);
	BOOST_CHECK ( map.isIn("toto") == true);
	BOOST_CHECK ( map.isIn("titi") == false);
	p = map.get("toto");
	BOOST_CHECK ( p.first == true );
	BOOST_CHECK ( p.second.tcp_endpoint ==  endpoint1);
	p = map.get("titi");
	BOOST_CHECK (p.first == false);

	addr.tcp_endpoint = endpoint2;
	BOOST_CHECK (map.add("toto", addr) == false);
	BOOST_CHECK ( map.isIn("toto") == true);
	p = map.get("toto");
	BOOST_CHECK ( p.first == true );
	BOOST_CHECK ( p.second.tcp_endpoint ==  endpoint1);

	BOOST_CHECK (map.remove("toto") == true);
	BOOST_CHECK (map.isIn("toto") == false);
	p = map.get("toto");
	BOOST_CHECK (p.first == false);

	// testing nameserver
	name_server s("127.0.0.1", "4242");
	boost::thread thr( boost::bind(& name_server::run, &s));
	typedef tcp::client<ns::output_msg> ns_client;

	boost::asio::io_service io_s;
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
	re.endpoint = endpoint1;

	c.request(re, rea);

	BOOST_CHECK(rea.name == re.name);
	BOOST_CHECK(rea.success == true);

	c.request(rn, rna);
	BOOST_CHECK(rna.name == rn.name);
	BOOST_CHECK(rna.success == true);
	BOOST_CHECK(rna.endpoint == endpoint1);

	s.stop();
	thr.join();
}
