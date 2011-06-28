#ifndef HYPER_MODEL_LOGIC_LAYER_HH
#define HYPER_MODEL_LOGIC_LAYER_HH

#include <list>

#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/thread.hpp>

#include <logic/engine.hh>
#include <logic/function_def.hh>
#include <model/logic_layer_fwd.hh>
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
			logic_layer_cb cb;

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

		namespace logic_layer_error {
			enum logic_layer_error_t {
				ok,
				parse_error,
				evaluation_error,
				recipe_execution_error,
				no_solution_found
			};
		}

		class logic_layer_category_impl
			  : public boost::system::error_category
		{
			public:
			  virtual const char* name() const;
			  virtual std::string message(int ev) const;
		};

		const boost::system::error_category& logic_layer_category();

		inline
		boost::system::error_code make_error_code(logic_layer_error::logic_layer_error_t e)
		{
			return boost::system::error_code(static_cast<int>(e), logic_layer_category());
		}

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
			void add_predicate(const std::string& s, const std::vector<std::string>& args_type);

			template <typename Func>
			void add_func(const std::string& s, const std::vector<std::string>& args_type);

			void async_exec(const logic_constraint& ctr, logic_layer_cb cb);
			void async_exec(const std::string& task, network::identifier id, 
							const std::string& src, logic_layer_cb cb);

			private:
			void handle_exec_computation(const boost::system::error_code&e,
										 logic_ctx_ptr logic_ctx);
			void handle_eval_task_tree(bool success, logic_ctx_ptr ptr);
			void handle_exec_task_tree(bool success, logic_ctx_ptr ptr);
		};
	}
}

namespace boost { namespace system {
	template <>
		struct is_error_code_enum<hyper::model::logic_layer_error::logic_layer_error_t>
		: public true_type {};
}}


#endif /* HYPER_MODEL_LOGIC_LAYER_HH */
