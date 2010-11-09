#ifndef HYPER_MODEL_LOGIC_LAYER_HH
#define HYPER_MODEL_LOGIC_LAYER_HH

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include <logic/engine.hh>
#include <model/concurrent_queue.hh>

namespace hyper {
	namespace model {
		struct ability;

		struct logic_constraint
		{
			std::string srcAbility;
			std::string constraint;
			size_t id;
		};

		typedef concurrent_queue<logic_constraint> logic_queue;

		namespace details {
			struct logic_solver {
				ability& a_;
				logic_queue& queue_;
				bool abort;
				logic::engine& engine_;

				logic_solver(logic_queue& queue__, ability& a,
						logic::engine& engine) :
					a_(a), queue_(queue__), abort(false),
					engine_(engine)
				{}

				void run();
			};
		}

		struct logic_layer {
			logic_queue& queue_;
			logic::engine engine;
			details::logic_solver solver;
			boost::thread thr;

			explicit logic_layer(logic_queue& queue, ability &a) :
				queue_(queue),
				engine(),
				solver(queue, a, engine),
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
