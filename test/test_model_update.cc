#define HYPER_UNIT_TEST
#include <model/ability_impl.hh>
#include <model/update_impl.hh>
#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>
#include <cmath>

#include <network/nameserver.hh>

namespace {
	struct pos_ability : public hyper::model::ability 
	{
		double x;
		int y, z;

		pos_ability() : hyper::model::ability("pos")
		{
			export_variable("x", x);
			export_variable("y", y);
			export_variable("z", z);
			start();
		}
	};

	struct goal_ability : public hyper::model::ability
	{
		int j;

		goal_ability() : hyper::model::ability("goal")
		{
			export_variable("j", j);
			start();
		}
	};

	struct update_test {
		hyper::model::updater update;
		pos_ability& pos;
		goal_ability& goal;
		size_t valid_test;

		update_test(pos_ability& pos, goal_ability & goal):
			update(pos), pos(pos), goal(goal), valid_test(0)
		{
			update.add("x"); // no update expected
			update.add("y", "task_y"); // expected the call of task_y
			update.add("z", "goal::j", pos.z); // sync z with goal::j
		};

		void handle_third_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(e == boost::asio::error::invalid_argument);
			valid_test++;
		}

		void handle_second_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			std::cout << "handle_second_test " << e << std::endl;
			BOOST_CHECK(pos.x == 42.0);
			BOOST_CHECK(pos.y == 3);
			BOOST_CHECK(pos.z == 42);
			valid_test++;

			// update a non existing variable
			update.async_update("pipo", 2, "test", 
					boost::bind(&update_test::handle_third_test,
								this,
								boost::asio::placeholders::error));
		}

		void handle_first_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(pos.x == 42.0);
			BOOST_CHECK(pos.y == 3);
			BOOST_CHECK(pos.z == 0);
			valid_test++;

			goal.j = 42;
			update.async_update("z", 1, "test",
					boost::bind(&update_test::handle_second_test, 
						this,
						boost::asio::placeholders::error));
		}

		void test_async()
		{
			pos.x = 42.0;
			pos.y = 3;
			pos.z = 0;
			update.async_update("x", 0, "test",
					boost::bind(&update_test::handle_first_test, 
						this,
						boost::asio::placeholders::error));
		}
	};
}


BOOST_AUTO_TEST_CASE ( model_update_test )
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

	update_test test(pos_, goal_);
	test.test_async();

	// sleep a bit to be sure that everything happens
	boost::this_thread::sleep(boost::posix_time::milliseconds(10)); 
	BOOST_CHECK(test.valid_test == 3);

	goal_.stop();
	thr3.join();
	pos_.stop();
	thr2.join();
	s.stop();
	thr.join();
}
