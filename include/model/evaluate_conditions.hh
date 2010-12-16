#ifndef HYPER_MODEL_EVALUATE_CONDITIONS_HH_
#define HYPER_MODEL_EVALUATE_CONDITIONS_HH_

#include <algorithm>
#include <deque>
#include <numeric>
#include <string>
#include <vector>

#include <model/eval_conditions_fwd.hh>
#include <model/task_fwd.hh>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace hyper {
	namespace model {

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

	}
}

#endif /* HYPER_MODEL_EVALUATE_CONDITIONS_HH_ */
