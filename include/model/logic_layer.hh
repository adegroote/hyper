#ifndef HYPER_MODEL_LOGIC_LAYER_HH
#define HYPER_MODEL_LOGIC_LAYER_HH

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include <model/concurrent_queue.hh>

namespace hyper {
	namespace model {
		struct logic_constraint
		{
			std::string srcAbility;
			std::string constraint;
			size_t id;
		};

		typedef concurrent_queue<logic_constraint> logic_queue;

		namespace details {
			struct logic_solver {
				logic_queue& queue_;
				bool abort;

				logic_solver(logic_queue& queue__) :
					queue_(queue__), abort(false)
				{}

				void run()
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
			};
		}

		struct logic_layer {
			logic_queue& queue_;
			details::logic_solver solver;
			boost::thread thr;

			explicit logic_layer(logic_queue& queue) :
				queue_(queue),
				solver(queue),
				thr(boost::bind(&details::logic_solver::run, &solver))
			{}

			~logic_layer() 
			{
				solver.abort = true;
				queue_.push(logic_constraint());
				thr.join();
			}
		};
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_HH */
