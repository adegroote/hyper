#ifndef HYPER_MODEL_LOGIC_LAYER_HH
#define HYPER_MODEL_LOGIC_LAYER_HH

#include <list>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include <logic/engine.hh>
#include <logic/function_def.hh>
#include <model/task.hh>


namespace hyper {
	namespace model {
		struct ability;

		struct logic_constraint
		{
			std::string srcAbility;
			std::string constraint;
			size_t id;
		};

		struct logic_context
		{
			logic_constraint ctr;	

			/* Context for executing remote rqst */
			logic::function_call call_exec;
			boost::optional<bool> exec_res;

			/* Logic layer evaluation */
			task_evaluation_seq seqs;

			/* More to come */
			logic_context(const logic_constraint& ctr_) : ctr(ctr_) {}
		};

		typedef boost::shared_ptr<logic_context> logic_ctx_ptr;

		struct logic_layer {
			logic::engine engine;
			ability& a_;
			std::map<std::string, task_ptr> tasks;
			
			logic::funcDefList execFuncs;

			logic_layer(ability &a);
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

			void async_exec(const logic_constraint& ctr);

			/* XXX */
			void handle_evaluation_preconds(logic_ctx_ptr ctx, 
				const std::string& name, conditionV failed);

			private:
			void handle_exec_computation(const boost::system::error_code&e,
										 logic_ctx_ptr logic_ctx);
			void compute_potential_task(logic_ctx_ptr ctx);
		};
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_HH */
