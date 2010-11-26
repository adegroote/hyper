#ifndef _HYPER_MODEL_TASK_HH_
#define _HYPER_MODEL_TASK_HH_

#include <numeric>

#include <network/nameserver.hh>
#include <network/proxy.hh>

#include <logic/expression.hh>

#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/shared_ptr.hpp>

namespace hyper {
	namespace model {

		struct ability;

		typedef std::vector<std::string> conditionV;
		typedef boost::function<void (conditionV)> condition_execution_callback;

		struct task
		{
			const ability& a;
			std::string name;

			task(const ability& a_, const std::string& name_) 
				: a(a_), name(name_) {}

			virtual void async_evaluate_preconditions(condition_execution_callback cb) = 0;

			virtual ~task() {};
		};

		typedef boost::shared_ptr<task> task_ptr;

		/*
		 * Expression will be sub-classed to build real computational
		 * expression on top of a specification 
		 * */



		template <int N, typename A>
		class evaluate_conditions {
			public:
				typedef boost::function<void (const A&, 
											  bool&, 
											  evaluate_conditions<N, A>&,
											  size_t)> fun_call;

				typedef std::pair<fun_call, std::string> condition;

			private:
				A& a;
				bool is_computing;
				boost::array<bool, N> terminated;
				boost::array<bool, N> success;
				boost::array<condition, N> condition_calls;
				std::vector<condition_execution_callback> callbacks;

				bool is_terminated() const
				{
					return std::accumulate(terminated.begin(), terminated.end(),
							true, std::logical_and<bool>());
				}

				struct reset {
					void operator() (bool& b) const { b = false; }
				};

			public:
				evaluate_conditions(A& a_, 
									boost::array<condition, N> condition_calls_) :
					a(a_), is_computing(false), condition_calls(condition_calls_) 
				{}

				void async_compute(condition_execution_callback cb)
				{
					callbacks.push_back(cb);
					if (is_computing) 
						return;

					/* Reinit the state of the object */
					std::for_each(terminated.begin(), terminated.end(), reset());

					is_computing = true;
					for (size_t i = 0; i != N; ++i) {
						a.io_s.post(boost::bind(condition_calls[i].first, boost::cref(a), 
															  boost::ref(success[i]),
															  boost::ref(*this), i));
					}
				}

				void handle_computation(size_t i)
				{
					terminated[i] = true;

					if (!is_terminated())
						return;

					// generate a vector for condition not enforced
					conditionV res;
					for (size_t j = 0; j != N; ++j) {
						if (!success[j])
							res.push_back(condition_calls[j].second);
					}

					// for each callback call it
					for (size_t j = 0; j < callbacks.size(); ++j) {
						a.io_s.post(boost::bind(callbacks[j],res));
					}

					is_computing = false;
					callbacks.clear();
				}
		};

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
