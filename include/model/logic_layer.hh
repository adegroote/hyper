#ifndef HYPER_MODEL_LOGIC_LAYER_HH
#define HYPER_MODEL_LOGIC_LAYER_HH

#include <list>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include <logic/engine.hh>
#include <logic/function_def.hh>
#include <model/task_fwd.hh>
#include <model/compute_task_tree.hh>

namespace hyper {
	namespace model {
		struct ability;

		struct logic_constraint
		{
			std::string constraint;
			size_t id;
			std::string src;
		};

		inline
		std::ostream& operator << (std::ostream& oss, 
								   const logic_constraint& c)
		{
			oss << "[" << c.src << ", " << c.id << "]";
			return oss;
		}

		struct logic_context
		{
			logic_constraint ctr;	

			/* Context for executing remote rqst */
			logic::function_call call_exec;
			boost::optional<bool> exec_res;

			/* Logic layer evaluation */
			compute_task_tree logic_tree;

			/* More to come */
			logic_context(logic_layer& logic) : logic_tree(logic, *this) {}
			logic_context(const logic_constraint& ctr_, logic_layer& logic) : 
				ctr(ctr_), logic_tree(logic, *this) {}
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
			void async_exec(const std::string& task, network::identifier id, const std::string& src);

			private:
			void handle_exec_computation(const boost::system::error_code&e,
										 logic_ctx_ptr logic_ctx);
			void handle_eval_task_tree(bool success, logic_ctx_ptr ptr);
		};
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_HH */
