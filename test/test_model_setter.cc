#define HYPER_UNIT_TEST
#include <model/ability_impl.hh>
#include <model/setter_impl.hh>
#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>
#include <cmath>

#include <network/nameserver.hh>

namespace {
	struct localisation {
		private:
			friend class boost::serialization::access;
			template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				(void) version;
				ar & x & y;
			}
		public:
			int x, y;
	};

	struct pos_ability : public hyper::model::ability 
	{
		hyper::model::setter setter;
		double x;
		int y, z;
		localisation pos;

		pos_ability() : hyper::model::ability("pos"), setter(*this)
		{
			setter.add("x", x);
			setter.add("y", y);
			setter.add("z", z);
			setter.add("pos", pos);

			start();
		}
	};

	struct goal_ability : public hyper::model::ability
	{
		int j;
		localisation g;

		goal_ability() : hyper::model::ability("goal")
		{
			export_variable("j", j);
			export_variable("g", g);

			start();
		}
	};

	struct setter_test {

		pos_ability& pos;
		goal_ability& goal;
		int valid_test;

		std::string current_var;
		boost::optional<hyper::logic::expression> current_expr;

		setter_test(pos_ability& pos, goal_ability & goal):
			pos(pos), goal(goal), valid_test(0)
		{
		};

		void handle_third_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(pos.pos.x == 743);
			BOOST_CHECK(pos.pos.y = 522);
			BOOST_CHECK(goal.g.x == 743);
			BOOST_CHECK(goal.g.y == 522);
			valid_test++;
		}


		void handle_second_test(const boost::system::error_code& e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(pos.x == 42.0);
			BOOST_CHECK(pos.y == 72);
			BOOST_CHECK(pos.z == 0);
			valid_test++;

			pos.pos.x = 0.0;
			pos.pos.y = 0.0;
			goal.g.x = 743;
			goal.g.y = 522;

			current_var = "pos";
			current_expr = hyper::logic::generate_node("goal::g");
			BOOST_CHECK(current_expr);

			pos.setter.set(current_var, *current_expr, 
					boost::bind(&setter_test::handle_third_test,
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

			goal.j = 72.0;

			current_var = "y";
			current_expr = hyper::logic::generate_node("goal::j");
			BOOST_CHECK(current_expr);

			pos.setter.set(current_var, *current_expr, 
					boost::bind(&setter_test::handle_second_test,
								this, 
								boost::asio::placeholders::error));
		}

		void test_async()
		{
			pos.x = 0.0;
			pos.y = 3;
			pos.z = 0;

			current_var = "x";
			current_expr = hyper::logic::generate_node("42.0");
			BOOST_CHECK(current_expr);

			pos.setter.set(current_var, *current_expr, 
					boost::bind(&setter_test::handle_first_test,
								this, 
								boost::asio::placeholders::error));
		}
	};
}


BOOST_AUTO_TEST_CASE ( model_setter_test )
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

	setter_test test(pos_, goal_);
	test.test_async();

	// sleep a bit to be sure that everything happens
	boost::this_thread::sleep(boost::posix_time::milliseconds(20)); 
	BOOST_CHECK(test.valid_test == 3);

	goal_.stop();
	thr3.join();
	pos_.stop();
	thr2.join();
	s.stop();
	thr.join();
}
