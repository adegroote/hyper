#include <logic/expression.hh>

#include <model/ability.hh>
#include <model/execute_impl.hh>
#include <model/logic_layer.hh>

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

					logic::generate_return ret =
						logic::generate(ctr.constraint, engine_.funcs());

					if (ret.res == false) {
						std::cerr << "Can't parse constraint" << std::endl;
						continue;
					}

					boost::optional<bool> b =
						model::evaluate_expression<bool>(a_.io_s, ret.e, a_);

					if (b && *b) {
						std::cout << "Constraint is already enforced, doing nothing";
						std::cout << std::endl;
						continue;
					}

					std::cout << "Time to do something smart" << std::endl;
				}
			}
		}
	}
}

