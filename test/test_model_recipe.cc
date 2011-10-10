#include <model/ability_impl.hh>
#include <model/task.hh>
#include <model/recipe.hh>
#include <boost/test/unit_test.hpp>

#include <boost/thread.hpp>
#include <cmath>

#include <network/nameserver.hh>

namespace {
	using namespace hyper::model;

	struct pos_ability : public hyper::model::ability 
	{
		int x, y, z;

		pos_ability() : hyper::model::ability("pos")
		{
			export_variable("x", x);
			export_variable("y", y);
			export_variable("z", z);
		}
	};
	
	struct my_task : public hyper::model::task 
	{
		my_task(hyper::model::ability& a) :
			hyper::model::task(a, "my_task") {}

		// uselesss for this test, only need the error_context
		void async_evaluate_preconditions(hyper::model::condition_execution_callback cb)
		{}
		void async_evaluate_postconditions(hyper::model::condition_execution_callback cb)
		{}
		bool has_postconditions() const { return false; } 
	};

	struct add_recipe : public hyper::model::recipe
	{
		pos_ability& pos;

		add_recipe(pos_ability& pos, my_task& t) : recipe("add", pos, t), pos(pos) {}

		virtual void async_evaluate_preconditions(condition_execution_callback cb) 
		{
			cb(boost::system::error_code(), hyper::model::conditionV());
		}

		virtual void do_execute(abortable_computation::cb_type cb)
		{
			pos.z = pos.x + pos.y;
			cb(boost::system::error_code());
		}

		virtual size_t nb_preconditions() const { return 0; }
	};

	struct mult_recipe : public hyper::model::recipe
	{
		pos_ability& pos;

		mult_recipe(pos_ability& pos, my_task& t) : recipe("mult", pos, t), pos(pos) {}

		virtual void async_evaluate_preconditions(condition_execution_callback cb) 
		{
			cb(boost::system::error_code(), hyper::model::conditionV());
		}

		virtual void do_execute(abortable_computation::cb_type cb)
		{
			pos.z = pos.x * pos.y;
			cb(boost::system::error_code());
		}

		virtual size_t nb_preconditions() const { return 0; }
	};

	struct div_recipe : public hyper::model::recipe
	{
		pos_ability& pos;

		div_recipe(pos_ability& pos, my_task& t) : recipe("div", pos, t), pos(pos) {}

		virtual void async_evaluate_preconditions(condition_execution_callback cb) 
		{
			hyper::model::conditionV error;
			error.push_back(hyper::logic::function_call());
			cb(boost::system::error_code(), error);
		}

		virtual void do_execute(abortable_computation::cb_type cb)
		{
			pos.z = pos.x / pos.y;
			cb(boost::system::error_code());
		}
		virtual size_t nb_preconditions() const { return 0; }
	};

	struct recipe_test {
		pos_ability& pos;
		my_task t;
		size_t valid_test;
		add_recipe add_;
		mult_recipe mult_;
		div_recipe div_;

		recipe_test(pos_ability& pos):
			pos(pos), t(pos), valid_test(0),
			add_(pos, t), mult_(pos, t), div_(pos, t)
		{
		};

		void handle_second_test(boost::optional<hyper::logic::expression> e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(pos.x == 42);
			BOOST_CHECK(pos.y == 3);
			BOOST_CHECK(pos.z == 42 * 3);
			valid_test++;
		}

		void handle_first_test(boost::optional<hyper::logic::expression> e)
		{
			BOOST_CHECK(!e);
			BOOST_CHECK(pos.x == 42);
			BOOST_CHECK(pos.y == 3);
			BOOST_CHECK(pos.z == 45);
			valid_test++;

			mult_.execute(boost::bind(&recipe_test::handle_second_test, this, _1));
		}

		void test_async()
		{
			pos.x = 42;
			pos.y = 3;
			pos.z = 0;

			add_.execute(boost::bind(&recipe_test::handle_first_test, this, _1));
		}
	};
}


BOOST_AUTO_TEST_CASE ( model_recipe_test )
{
	using namespace hyper::model;
	using namespace hyper::network;
	using namespace hyper::logic;

	boost::asio::io_service io_nameserv_s;
	name_server s("127.0.0.1", "4242", io_nameserv_s, false);
	boost::thread thr( boost::bind(& boost::asio::io_service::run, &io_nameserv_s));

	pos_ability pos_;
	boost::thread thr2( boost::bind(& pos_ability::test_run, &pos_));

	recipe_test test(pos_);
	test.test_async();

	// sleep a bit to be sure that everything happens
	boost::this_thread::sleep(boost::posix_time::milliseconds(10)); 
	BOOST_CHECK(test.valid_test == 2);

	pos_.stop();
	thr2.join();
	s.stop();
	thr.join();
}
