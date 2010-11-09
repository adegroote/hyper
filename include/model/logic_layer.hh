#ifndef HYPER_MODEL_LOGIC_LAYER_HH
#define HYPER_MODEL_LOGIC_LAYER_HH

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include <logic/engine.hh>
#include <logic/function_def.hh>
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
				logic::funcDefList& execFuncs_;

				logic_solver(logic_queue& queue__, ability& a,
						logic::engine& engine,
						logic::funcDefList& execFuncs) :
					a_(a), queue_(queue__), abort(false),
					engine_(engine), execFuncs_(execFuncs)
				{}

				void run();
			};
		}

		struct logic_layer {
			logic_queue& queue_;
			logic::engine engine;
			ability& a_;

			logic::funcDefList execFuncs;
			details::logic_solver solver;

			boost::thread thr;

			logic_layer(logic_queue& queue, ability &a);
			~logic_layer();

			template <typename T>
			void add_equalable_type(std::string s); 
			template <typename T>
			void add_numeric_type(std::string s);
			template <typename T>
			void add_comparable_type(std::string s);

			template <typename Func>
			void add_predicate(const std::string& s);

			template <typename Func>
			void add_func(const std::string& s);
		};
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_HH */
