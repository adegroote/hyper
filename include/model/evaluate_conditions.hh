#ifndef HYPER_MODEL_EVALUATE_CONDITIONS_HH_
#define HYPER_MODEL_EVALUATE_CONDITIONS_HH_

#include <algorithm>
#include <deque>
#include <numeric>
#include <string>
#include <vector>

#include <network/proxy.hh>
#include <model/eval_conditions_fwd.hh>
#include <model/update_impl.hh>
#include <model/task_fwd.hh>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/utility/enable_if.hpp>

namespace hyper {
	namespace model {
		/*
		 * N corresponds to the number of conditions to compute
		 * A is the associated ability model
		 * M is the number of local_variables to update
		 * vectorT is the list of type of remote variables we need to retrieve
		 */
		template <int N, typename A, int M = 0, typename vectorT=boost::mpl::vector<> >
		class evaluate_conditions {
			public:
				typedef boost::function<void (const A&, 
											  bool&, 
											  evaluate_conditions<N, A, M, vectorT>&,
											  size_t)> fun_call;

				typedef std::pair<fun_call, std::string> condition;

				typedef network::actor::remote_values<vectorT> remote_values;
				typedef update_variables<A, M, vectorT> updater_type;
				updater_type updater;

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

				void handle_update(const boost::system::error_code& e)
				{
					// XXX deal correctly with e
					for (size_t i = 0; i != N; ++i) {
						a.io_s.post(boost::bind(condition_calls[i].first, boost::cref(a), 
															  boost::ref(success[i]),
															  boost::ref(*this), i));
					}
				}

			public:
				evaluate_conditions(A& a_, 
									boost::array<condition, N> condition_calls_) :
					a(a_), is_computing(false), condition_calls(condition_calls_) 
				{}

				evaluate_conditions(A& a_, 
									boost::array<condition, N> condition_calls_,
									boost::array<std::string, M> update_status_) :
					a(a_), is_computing(false), condition_calls(condition_calls_),
					updater(a, update_status_)
				{}


				evaluate_conditions(A& a_, 
									boost::array<condition, N> condition_calls_,
									const typename remote_values::remote_vars_conf& vars):
					a(a_), is_computing(false), 
					condition_calls(condition_calls_), 
					updater(a, vars)
				{}

				evaluate_conditions(A& a_, 
									boost::array<condition, N> condition_calls_,
									boost::array<std::string, M> update_status_,
									const typename remote_values::remote_vars_conf& vars):
					a(a_), is_computing(false), 
					condition_calls(condition_calls_), 
					updater(a, update_status_, vars)
				{}

				void async_compute(condition_execution_callback cb)
				{
					callbacks.push_back(cb);
					if (is_computing) 
						return;

					/* Reinit the state of the object */
					std::for_each(terminated.begin(), terminated.end(), reset());

					is_computing = true;

					updater.async_update(boost::bind(
								&evaluate_conditions::handle_update, this,
								boost::asio::placeholders::error));
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
