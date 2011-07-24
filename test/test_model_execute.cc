#define HYPER_UNIT_TEST
#include <model/ability_impl.hh>
#include <model/execute_impl.hh>
#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>
#include <boost/serialization/serialization.hpp>

#include <cmath>

#include <network/nameserver.hh>

namespace {
	struct do_nothing
	{
		void operator () (const boost::system::error_code&) {}
	};

	struct point {
		private:
			friend class boost::serialization::access;
			template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				(void) version;
				ar & x & y;
			}
		public:

			double x; 
			double y;
	};

	struct distance {
		typedef double result_type;
		typedef boost::mpl::vector<point, point> args_type;

		static double apply(const point& A, const point& B) 
		{
			double diff_x = B.x - A.x;
			double diff_y = B.y - A.y;
			return sqrt(diff_x * diff_x + diff_y * diff_y);
		};
	};

	template <typename T>
	struct add {
		typedef T result_type;
		typedef typename boost::mpl::vector<T, T> args_type;

		static T apply(const T& A, const T& B)
		{
			return A + B;
		}
	};

	template <typename T>
	struct square {
		typedef T result_type;
		typedef typename boost::mpl::vector<T> args_type;

		static T apply(const T& A)
		{
			return A * A;
		}
	};

	struct pos_ability : public hyper::model::ability 
	{
		point A;
		point B;
		double x;
		int y, z;

		pos_ability() : hyper::model::ability("pos")
		{
			export_variable("A", A);
			export_variable("B", B);
			export_variable("x", x);
			export_variable("y", y);
			export_variable("z", z);

			f_map.add("distance", new hyper::model::function_execution<distance>());
			f_map.add("add_int", new hyper::model::function_execution<add<int> >());
			f_map.add("square_int", new hyper::model::function_execution<square<int> >());

			start();
		}
	};

	struct goal_ability : public hyper::model::ability
	{
		point goal;

		goal_ability() : hyper::model::ability("goal")
		{
			export_variable("goal", goal);

			start();
		}
	};
}


BOOST_AUTO_TEST_CASE ( model_execute_test )
{
	using namespace hyper::model;
	using namespace hyper::network;
	using namespace hyper::logic;

	funcDefList f;
	f.add("distance", 2);
	f.add("add_int", 2);
	f.add("square_int", 1);

	boost::asio::io_service io_nameserv_s;
	name_server s("127.0.0.1", "4242", io_nameserv_s, true);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_nameserv_s));

	pos_ability pos_;
	boost::thread thr2( boost::bind(& pos_ability::test_run, &pos_));

	goal_ability goal_;
	boost::thread thr3 (boost::bind(& goal_ability::test_run, &goal_));

	generate_return r;
	r = generate("square_int(12)", f);
	BOOST_CHECK(r.res == true);

	boost::asio::io_service io_s;
	boost::optional<int> res = evaluate_expression<int>(io_s, r.e, pos_);
	BOOST_CHECK(res);
	BOOST_CHECK(*res == 12 * 12);

	r = generate("add_int(2, 2)", f);
	BOOST_CHECK(r.res == true);

	res = evaluate_expression<int>(io_s, r.e, pos_);
	BOOST_CHECK(res);
	BOOST_CHECK(*res == 2 + 2);
	
	r = generate("square_int(add_int(2, 2))", f);
	BOOST_CHECK(r.res == true);

	res = evaluate_expression<int>(io_s, r.e, pos_);
	BOOST_CHECK(res);
	BOOST_CHECK(*res == 16);

	r = generate("add_int(square_int(2), add_int(2, 2))", f);
	BOOST_CHECK(r.res == true);

	res = evaluate_expression<int>(io_s, r.e, pos_);
	BOOST_CHECK(res);
	BOOST_CHECK(*res == 8);

	pos_.y = 24;
	pos_.z = 18;
	r = generate("add_int(y, z)", f);
	BOOST_CHECK(r.res == true);

	res = evaluate_expression<int>(io_s, r.e, pos_);
	BOOST_CHECK(res);
	BOOST_CHECK(*res == 42);

	r = generate("add_int(x, z)", f);
	BOOST_CHECK(r.res == true);

	res = evaluate_expression<int>(io_s, r.e, pos_);
	BOOST_CHECK(!res); // x is of type double

	pos_.A.x = 0.0;
	pos_.A.y = 0.0;
	pos_.B.x = 10.0;
	pos_.B.y = 0.0;
	r = generate("distance(pos::A, pos::B)", f);
	BOOST_CHECK(r.res == true);

	boost::optional<double> res_distance;
	res_distance = evaluate_expression<double>(io_s, r.e, pos_);
	BOOST_CHECK(res_distance);
	BOOST_CHECK(std::abs(*res_distance - 10.0) < 1e-6);

	goal_.goal.x = 0.0;
	goal_.goal.y = 42.0;

	r = generate("distance(pos::A, goal::goal)", f);
	BOOST_CHECK(r.res == true);
	/* There is a small issue with the evaluate_expression,
	 * we run io_s. But in the remote case, we post a job for name client which
	 * is not on this io_service : the consequence is that io_s get out of job
	 * and so returns, without terminating its jobs.
	 *
	 * Adding a deadline time to workaround it. The normal use case is to use
	 * asynchronously, with lots of other jobs (and in real scenario,
	 * name_client uses the same name_service)
	 */
	boost::asio::deadline_timer deadline_(io_s);
	deadline_.expires_from_now(boost::posix_time::milliseconds(20));
	deadline_.async_wait(do_nothing());
	res_distance = evaluate_expression<double>(io_s, r.e, pos_);
	BOOST_CHECK(res_distance);
	if (res_distance) {
		BOOST_CHECK(std::abs(*res_distance - 42.0) < 1e-6);
	}

	goal_.stop();
	thr3.join();
	pos_.stop();
	thr2.join();
	s.stop();
	thr.join();
}
