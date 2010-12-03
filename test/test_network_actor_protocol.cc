#include <network/actor_protocol.hh>
#include <network/msg.hh>
#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>

using namespace hyper::network;

namespace {
	typedef boost::mpl::vector<request_constraint> input_serv;
	typedef boost::mpl::vector<boost::mpl::void_> output_serv;
	typedef boost::mpl::vector<request_constraint_answer> input_client;
	typedef boost::mpl::vector<boost::mpl::void_> output_client;
	typedef boost::make_variant_over< input_serv>::type input_serv_variant;
	typedef boost::make_variant_over< output_serv>::type output_serv_variant;
	typedef boost::make_variant_over< input_client>::type input_client_variant;
	typedef boost::make_variant_over< output_client>::type output_client_variant;

	struct false_resolv
	{
		false_resolv() {};

		template <typename Handler>
		void async_resolve(name_resolve & solv, Handler handler)
		{
			if (solv.name() == "client") {
			solv.rna.endpoint = boost::asio::ip::tcp::endpoint(
					  boost::asio::ip::address::from_string("127.0.0.1"), 5000);
			} else {
			solv.rna.endpoint = boost::asio::ip::tcp::endpoint(
					  boost::asio::ip::address::from_string("127.0.0.1"), 5001);
			}
			handler(boost::system::error_code());
		}
	};

	struct simple_agent {
		size_t identifier;
		std::string name;
		boost::asio::io_service io_s;
		false_resolv name_client;
		callback_database<input_client> db;

		size_t gen_identifier() { return identifier++; }
	};

	typedef hyper::network::tcp::server<input_client, output_client, actor_protocol_visitor<simple_agent>		>
		client_serv;

	void dont_care(const boost::system::error_code& ) {}

	struct simple_server_visitor : public boost::static_visitor<output_serv_variant>
	{
		request_constraint_answer ans;
		actor_client_database<simple_agent> & db;

		simple_server_visitor(actor_client_database<simple_agent> & db_) : db(db_) {}

		boost::mpl::void_ operator() (const request_constraint& r) 
		{
			ans.id = r.id;
			ans.success = true;
			db[r.src].async_write(ans, dont_care);

			return boost::mpl::void_();
		}
	};

	typedef hyper::network::tcp::server<input_serv, output_serv, simple_server_visitor>
		server_serv;

	struct agent_client {
		simple_agent agent;
		actor_client<simple_agent> client;
		actor_protocol_visitor<simple_agent> vis;
		client_serv serv;
		int count_valid_test;
		request_constraint req;
		request_constraint_answer ans;

		agent_client() : client(agent, "server"), 
			vis(agent), serv("127.0.0.1", "5000", vis, agent.io_s) {
			agent.name = "client";
			count_valid_test = 0;
		}

		void handle_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(ans.id == 42);
			BOOST_CHECK(ans.success == true);
			count_valid_test ++;
		}

		void stop() { serv.stop(); }

		void test_async()
		{
			agent.identifier = 42;
			req.constraint = "test";
			client.async_request(req, ans, boost::bind(&agent_client::handle_test, this,
										boost::asio::placeholders::error));
		}
	};

	struct agent_serv {
		simple_agent agent;
		actor_client_database<simple_agent> db;
		simple_server_visitor vis;
		server_serv serv;

		agent_serv() : db(agent),
			vis(db),
			serv("127.0.0.1", "5001", vis, agent.io_s) {
				agent.name = "server";
			}

		void stop() { serv.stop(); }
	};
}

BOOST_AUTO_TEST_CASE ( network_actor_protocol_test )
{
	agent_serv serv;
	agent_client client;

	boost::thread thr_serv( boost::bind(& boost::asio::io_service::run, &serv.agent.io_s));
	boost::thread thr_client( boost::bind(& boost::asio::io_service::run, &client.agent.io_s));

	client.test_async();
	// sleep a bit to be sure that everything happens
	boost::this_thread::sleep(boost::posix_time::milliseconds(10)); 

	serv.stop();
	client.stop();
	thr_serv.join();
	thr_client.join();

	BOOST_CHECK(client.count_valid_test == 1);
}
