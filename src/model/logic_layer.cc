#include <logic/expression.hh>
#include <logic/eval.hh>

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

	void add_transitivy_rule(hyper::logic::engine& e, const std::string& s)
	{
		std::string rule_name = s + "_transitivity";
		std::vector<std::string> cond;
		std::vector<std::string> action;
		
		cond.push_back(s + "(A,B)");
		cond.push_back(s + "(B,C)");
		action.push_back(s + "(A,C)");

		e.add_rule(rule_name, cond, action);
	}

	void add_symetry_rule(hyper::logic::engine& e, const std::string& s)
	{
		std::string rule_name = s + "_symetry";
		std::vector<std::string> cond;
		std::vector<std::string> action;
		
		cond.push_back(s + "(A,B)");

		std::ostringstream oss;
		oss << "equal(" << s << "(A,B), " << s << "(B, A))";
		action.push_back(oss.str());

		e.add_rule(rule_name, cond, action);
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
			/* Add exec func */
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

			/* Add logic func */
			engine.add_predicate("less", 2, new logic::eval<
											details::logic_eval<details::logic_less>, 2>());
			engine.add_predicate("less_equal", 2, new logic::eval<
											details::logic_eval<details::logic_less_equal>, 2>());
			engine.add_predicate("greater", 2, new logic::eval<
											details::logic_eval<details::logic_greater>, 2>());
			engine.add_predicate("greater_equal", 2, new logic::eval<
											details::logic_eval<details::logic_greater_equal>, 2>());

			engine.add_func("add", 2);
			engine.add_func("minus", 2);
			engine.add_func("times", 2);
			engine.add_func("divides", 2);
			engine.add_func("negate", 1);

			/* Add logic rules */
			add_transitivy_rule(engine, "less");
			add_transitivy_rule(engine, "less_equal");
			add_transitivy_rule(engine, "greater");
			add_transitivy_rule(engine, "greater_equal");

			add_symetry_rule(engine, "add");
			add_symetry_rule(engine, "mines");
			add_symetry_rule(engine, "times");
			add_symetry_rule(engine, "divides");
		}

		logic_layer::~logic_layer() 
		{
			solver.abort = true;
			queue_.push(logic_constraint());
			thr.join();
		}
	}
}

