#include <model/ability.hh>
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
					}
				}
		}
	}
}

