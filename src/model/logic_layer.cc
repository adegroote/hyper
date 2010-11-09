#include <logic/expression.hh>

#include <model/ability.hh>
#include <model/execute_impl.hh>
#include <model/logic_layer_impl.hh>

namespace {

	/* 
	 * Remove % if we found some pattern between % % 
	 * % will be used to denote the type of the expression
	 */
	std::string prepare_execution_rqst(const std::string& s)
	{
		std::string res = s;
		std::string::size_type loc, previous_loc;
		previous_loc = 0;

		while ( (loc = res.find("%", previous_loc)) != std::string::npos)
		{
			previous_loc = loc;
			loc = res.find("%", previous_loc+1);
			if (loc == std::string::npos) 
				break;

			/* We found a pattern between two % */
			res.erase(loc,1);
			res.erase(previous_loc,1);
			previous_loc = loc - 2;
		}

		return res;
	}

	/* 
	 * If we found a pattern between % %, remove it
	 * % will be used to denote the type of the expression, and so is useless
	 * in the logic world
	 */
	std::string prepare_logic_rqst(const std::string& s)
	{
		std::string res = s;
		std::string::size_type loc, previous_loc;
		previous_loc = 0;

		while ( (loc = res.find("%", previous_loc)) != std::string::npos)
		{
			previous_loc = loc;
			loc = res.find("%", previous_loc+1);
			if (loc == std::string::npos) 
				break;

			/* We found a pattern between two % */
			std::string::size_type pattern_size = loc - previous_loc;
			res.erase(previous_loc, pattern_size+1);
		}

		return res;
	}
}

namespace hyper {
	namespace model {
		namespace details {
			void logic_solver::run()
			{
				logic_constraint ctr;
				while (! abort)  {
					queue_.wait_and_pop(ctr);

					if (abort) 
						return;

					std::cout << "Handling constraint : " ;
					std::cout << ctr.constraint << std::endl;

					std::string to_execute = prepare_execution_rqst(ctr.constraint);

					logic::generate_return ret_exec =
						logic::generate(to_execute, execFuncs_);

					if (ret_exec.res == false) {
						std::cerr << "Can't parse constraint" << std::endl;
						continue;
					}

					boost::optional<bool> b =
						model::evaluate_expression<bool>(a_.io_s, ret_exec.e, a_);

					if (b && *b) {
						std::cout << "Constraint is already enforced, doing nothing";
						std::cout << std::endl;
						continue;
					}

					std::cout << "Time to do something smart" << std::endl;
				}
			}
		}


		logic_layer::logic_layer(logic_queue& queue, ability &a) :
			queue_(queue),
			engine(),
			a_(a),
			solver(queue, a, engine, execFuncs),
			thr(boost::bind(&details::logic_solver::run, &solver))
		{
			add_equalable_type<int>("int");
			add_equalable_type<double>("double");
			add_equalable_type<std::string>("string");
			add_equalable_type<bool>("bool");

			add_comparable_type<int>("int");
			add_comparable_type<double>("double");
			add_comparable_type<std::string>("string");
			add_comparable_type<bool>("bool");

			add_numeric_type<int>("int");
			add_numeric_type<double>("double");
		}

		logic_layer::~logic_layer() 
		{
			solver.abort = true;
			queue_.push(logic_constraint());
			thr.join();
		}
	}
}

