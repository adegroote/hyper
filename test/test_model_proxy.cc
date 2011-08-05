#define HYPER_UNIT_TEST
#include <model/ability_impl.hh>
#include <model/proxy.hh>
#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>
#include <boost/assign/list_of.hpp>
#include <cmath>

#include <network/nameserver.hh>

namespace {
	struct point {
		double x, y, z;

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			(void) version;
			ar & x & y & z;
		}
	};

	struct pos_ability : public hyper::model::ability 
	{
		int x;
		point P;

		pos_ability() : hyper::model::ability("pos")
		{
			start();
		}
	};

	struct goal_ability : public hyper::model::ability
	{
		int x;
		point P;

		goal_ability() : hyper::model::ability("goal")
		{
			export_variable("P", P);
			export_variable("x", x);
			start();
		}
	};

	struct proxy_test {
		hyper::model::remote_proxy proxy;
		pos_ability& pos;
		goal_ability& goal;
		size_t valid_test;
		boost::optional<int> x;
		boost::optional<point> P;
		hyper::model::remote_value<int> r_x;
		hyper::model::remote_value<point> r_P;
		typedef hyper::model::remote_values<boost::mpl::vector<int, point> > remote_values;
		remote_values r_;
		typedef hyper::model::remote_values<boost::mpl::vector<point, point, point> > remote_values_error;
		remote_values_error r_error;

		proxy_test(pos_ability& pos, goal_ability & goal):
			proxy(pos), pos(pos), goal(goal), valid_test(0),
			r_(remote_values::remote_vars_conf(boost::assign::list_of<std::pair<std::string, std::string> >
					("goal", "x")
					("goal", "P")
					)),
			r_error(remote_values_error::remote_vars_conf(boost::assign::list_of<std::pair<std::string, std::string> >
					("goal", "P")
					("goal", "x")
					("goal", "pipo")
					))
		{
		};

		void handle_eight_test(const boost::system::error_code &e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(r_error.is_terminated());
			BOOST_CHECK(r_error.at_c<0>());
			BOOST_CHECK(!r_error.at_c<1>());
			BOOST_CHECK(!r_error.at_c<2>());
			BOOST_CHECK((*r_error.at_c<0>()).x == 11.0);
			BOOST_CHECK((*r_error.at_c<0>()).y == 22.0);
			BOOST_CHECK((*r_error.at_c<0>()).z == 33.0);
			valid_test++;
		}

		void handle_seventh_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(!x);
			valid_test++;

			proxy.async_get(r_error, boost::bind(&proxy_test::handle_eight_test,
												this,
												boost::asio::placeholders::error));
		}

		// for a non-existing variable, we don't expect a system error, but the
		// resulting variable is set to boost::none
		void handle_sixth_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(!x);
			valid_test++;

			// check the case where the type of the variable is not the good one
			proxy.async_get("goal", "x", P, boost::bind(&proxy_test::handle_seventh_test,
														this,
														boost::asio::placeholders::error));
		}

		void handle_fifth_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(r_.is_terminated() == true);
			BOOST_CHECK(r_.at_c<0>());
			BOOST_CHECK(r_.at_c<1>());
			BOOST_CHECK(*r_.at_c<0>() == 42);
			BOOST_CHECK((*r_.at_c<1>()).x == 11.0);
			BOOST_CHECK((*r_.at_c<1>()).y == 22.0);
			BOOST_CHECK((*r_.at_c<1>()).z == 33.0);
			valid_test++;

			// check a non-existing variable
			proxy.async_get("goal", "z", x, boost::bind(&proxy_test::handle_sixth_test,
													    this,
														boost::asio::placeholders::error));
		}

		void handle_fourth_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(r_P.terminated == true);
			BOOST_CHECK(r_P.value);
			BOOST_CHECK((*r_P.value).x == 11.0);
			BOOST_CHECK((*r_P.value).y == 22.0);
			BOOST_CHECK((*r_P.value).z == 33.0);
			valid_test++;

			proxy.async_get(r_, boost::bind(&proxy_test::handle_fifth_test,
											this,
											boost::asio::placeholders::error));
		}

		void handle_third_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(r_x.terminated == true);
			BOOST_CHECK(r_x.value);
			BOOST_CHECK(*(r_x.value) == 42);
			valid_test++;

			r_P.src = "goal";
			r_P.msg.var_name = "P";
			r_P.value = boost::none;
			r_P.terminated = false;
			proxy.async_get(r_P, boost::bind(&proxy_test::handle_fourth_test,
											 this,
											 boost::asio::placeholders::error));
		}

		void handle_second_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(P);
			BOOST_CHECK((*P).x == 11.0);
			BOOST_CHECK((*P).y == 22.0);
			BOOST_CHECK((*P).z == 33.0);
			valid_test++;

			r_x.src = "goal";
			r_x.msg.var_name = "x";
			r_x.value = 22.0;
			r_x.terminated = false;
			proxy.async_get(r_x, boost::bind(&proxy_test::handle_third_test,
											 this,
											 boost::asio::placeholders::error));
		}

		void handle_first_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(x);
			BOOST_CHECK(*x == 42);
			valid_test++;

			goal.P.x = 11.0;
			goal.P.y = 22.0;
			goal.P.z = 33.0;
			proxy.async_get("goal", "P", P, 
					boost::bind(&proxy_test::handle_second_test, 
						this,
						boost::asio::placeholders::error));
		}

		void test_async()
		{
			goal.x = 42;
			proxy.async_get("goal", "x", x, 
					boost::bind(&proxy_test::handle_first_test, 
						this,
						boost::asio::placeholders::error));
		}
	};
}


BOOST_AUTO_TEST_CASE ( model_proxy_test )
{
	using namespace hyper::model;
	using namespace hyper::network;
	using namespace hyper::logic;

	boost::asio::io_service io_nameserv_s;
	name_server s("127.0.0.1", "4242", io_nameserv_s, false);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_nameserv_s));

	pos_ability pos_;
	boost::thread thr2( boost::bind(& pos_ability::test_run, &pos_));

	goal_ability goal_;
	boost::thread thr3 (boost::bind(& goal_ability::test_run, &goal_));

	proxy_test test(pos_, goal_);
	test.test_async();

	// sleep a bit to be sure that everything happens
	boost::this_thread::sleep(boost::posix_time::milliseconds(20)); 
	BOOST_CHECK(test.valid_test == 8);

	goal_.stop();
	thr3.join();
	pos_.stop();
	thr2.join();
	s.stop();
	thr.join();
}
