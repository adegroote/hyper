#ifndef _HYPER_MODEL_TASK_HH_
#define _HYPER_MODEL_TASK_HH_

#include <string>

#include <logic/expression.hh>
#include <model/logic_layer_fwd.hh>
#include <model/task_fwd.hh>
#include <model/recipe_fwd.hh>
#include <network/nameserver.hh>
#include <network/proxy.hh>

#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/shared_ptr.hpp>


namespace hyper {
	namespace model {

		struct ability;

		class task
		{
			private:
				ability& a;
				std::string name;
				typedef std::vector<recipe_ptr> vect_recipes;
				vect_recipes recipes;

				struct state {
					size_t index; // index in vect_recipes
					size_t nb_preconds; // nb of preconditions
					conditionV failed; // failed precondition

					bool operator< (const state& s) const
					{
						if (failed.size() != s.failed.size())
							return (failed.size() < s.failed.size());
						return nb_preconds > s.nb_preconds;
					}
				};

				std::vector<state> recipe_states;

				bool is_running;
				std::vector<task_execution_callback> pending_cb;
			public:
				task(ability& a_, const std::string& name_) 
					: a(a_), name(name_), is_running(false) {}

				void add_recipe(recipe_ptr ptr);
				virtual void async_evaluate_preconditions(condition_execution_callback cb) = 0;
				virtual void async_evaluate_postconditions(condition_execution_callback cb) = 0;
				void execute(task_execution_callback cb);
				virtual ~task() {};

			protected:
				virtual bool has_postconditions() const = 0;

			private:
				void handle_initial_postcondition_handle(conditionV failed);
				void handle_precondition_handle(conditionV failed);
				void handle_execute(bool res);
				void handle_final_postcondition_handle(conditionV failed);
				void async_evaluate_recipe_preconditions(conditionV, size_t i);
				void end_execute(bool res);
		};


		/*
		 * Expression will be sub-classed to build real computational
		 * expression on top of a specification 
		 * */
		template <typename vectorT>
		class expression
		{
			public:
			typedef network::remote_proxy_n<vectorT, network::name_client> r_proxy_n;
			typedef boost::array<std::pair<std::string, std::string>, r_proxy_n::size> arg_list;
	
			r_proxy_n r_proxy;

			expression(boost::asio::io_service& io_s,
						    const arg_list& l,
							network::name_client &r) :
				r_proxy(io_s, l, r) {}
			;
	
			private:
			template <typename Handler>
			void handle_proxy_get(boost::tuple<Handler> handler)
			{
				bool res = r_proxy.is_successful();
				if (res) 
					res = real_compute();
				boost::get<0>(handler)(res);
			}
	
			public:
			template <typename Handler>
			void compute(Handler handler)
			{
				void (expression::*f) (boost::tuple<Handler>)
					= &expression::template handle_proxy_get<Handler>;
				r_proxy.async_get(boost::bind(f, boost::make_tuple(handler)));
			}

			protected:
			virtual bool real_compute() const { return false; }
		};

		template <>
		struct expression<boost::mpl::vector<> >
		{
			public:
			expression() {}
	
			template <typename Handler>
			void compute(Handler handler)
			{
				bool res = real_compute();
				handler(res);
			}

			protected:
			virtual bool real_compute() const { return false; };
		};
	}
}

#endif /* _HYPER_MODEL_TASK_HH_ */
