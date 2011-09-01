#ifndef HYPER_MODEL_LOGIC_LAYER_HH
#define HYPER_MODEL_LOGIC_LAYER_HH

#include <list>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>

#include <logic/engine.hh>
#include <logic/function_def.hh>
#include <model/logic_layer_fwd.hh>
#include <model/task_fwd.hh>
#include <model/compute_task_tree.hh>
#include <network/types.hh>

namespace hyper {
	namespace model {
		struct ability;

		typedef std::pair<std::string, std::string> unification_pair;
		typedef std::pair<logic::expression, logic::expression> unification_pair2;
		typedef std::pair<std::string, logic::expression> unification_expr;
		typedef std::vector<unification_pair> unify_pair_list;
		typedef std::vector<unification_pair2> unify_pair_list2;
		typedef std::vector<unification_expr> unify_expr_list;

		struct logic_constraint
		{
			size_t id;
			std::string src;
			bool repeat;
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

			/* Unification context */
			unify_expr_list unify_list;

			/* Context for executing remote rqst */
			logic::function_call call_exec;
			boost::optional<bool> exec_res;

			/* Logic layer evaluation */
			compute_task_tree logic_tree;

			boost::asio::deadline_timer deadline_;

			enum state { IDLE, EXEC, LOGIC, LOGIC_EXEC, WAIT };

			bool must_interrupt;
			state s_;

			/* More to come */
			logic_context(logic_layer& logic); 
			logic_context(const logic_constraint& ctr_, logic_layer& logic);  
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
			std::map<std::string, logic_ctx_ptr> running_ctx;

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

			void add_rules(const std::string& s, const std::vector<logic::function_call>& premises, 
												 const std::vector<logic::function_call>& conclusions)
			{
				engine.add_rule(s, premises, conclusions);
			}

			void async_exec(const logic_constraint& ctr, 
							const std::string& constraint, 
							const unify_pair_list&, logic_layer_cb cb);
			void async_exec(const logic_constraint& ctr, 
							const logic::function_call& constraint, 
							const unify_pair_list2&, logic_layer_cb cb);
			void async_exec(const std::string& task, network::identifier id, 
							const std::string& src, logic_layer_cb cb);

			void abort(const std::string& src, network::identifier id);

			private:
			void handle_unification_computation(const boost::system::error_code& e,
												logic_ctx_ptr logic_ctx);
			void handle_exec_computation(const boost::system::error_code&e,
										 logic_ctx_ptr logic_ctx);
			void handle_eval_task_tree(bool success, logic_ctx_ptr ptr);
			void handle_exec_task_tree(bool success, logic_ctx_ptr ptr);
			void handle_success(logic_ctx_ptr ptr);
			void handle_failure(logic_ctx_ptr ptr, const boost::system::error_code&);
			void handle_timeout(const boost::system::error_code&, logic_ctx_ptr);

			std::string make_key(const std::string& src, network::identifier id) const;
			std::string make_key(logic_ctx_ptr ptr) const;

			logic_ctx_ptr prepare_async_exec(const logic_constraint& ctr, logic_layer_cb cb);
			void async_exec_(logic_ctx_ptr ctx);
		};
	}
}

namespace boost { namespace system {
	template <>
		struct is_error_code_enum<hyper::model::logic_layer_error::logic_layer_error_t>
		: public true_type {};
}}


#endif /* HYPER_MODEL_LOGIC_LAYER_HH */
