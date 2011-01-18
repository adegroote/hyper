#ifndef HYPER_MODEL_EVALUATE_CONDITIONS_HH_
#define HYPER_MODEL_EVALUATE_CONDITIONS_HH_

#include <algorithm>
#include <deque>
#include <numeric>
#include <string>
#include <vector>

#include <network/proxy.hh>
#include <model/eval_conditions_fwd.hh>
#include <model/task_fwd.hh>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/mpl/vector.hpp>

namespace hyper {
	namespace model {
		namespace details {
			struct updating_value { std::string value; bool terminated; };

			template <int N, typename A, typename vectorT, int M>
			struct local_update;
		}

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

				remote_values remote;
			private:

				A& a;
				bool is_computing;
				boost::array<bool, N> terminated;
				boost::array<bool, N> success;
				boost::array<condition, N> condition_calls;
				boost::array<details::updating_value, M> updating_status;
				std::vector<condition_execution_callback> callbacks;

				bool is_terminated() const
				{
					return std::accumulate(terminated.begin(), terminated.end(),
							true, std::logical_and<bool>());
				}


				struct reset {
					void operator() (bool& b) const { b = false; }
				};

				void check_all_variables_up2date()
				{
					bool local_terminaison = true;
					bool remote_terminaison = true;

					for (size_t i = 0; i != M; i++)
						local_terminaison &= updating_status[i].terminated;

					if (boost::mpl::size<vectorT>::type::value)
						remote_terminaison = remote.is_terminated();

					if (!(local_terminaison && remote_terminaison))
						return; // wait for completion of updating
					
					// every variables are up 2 date, just compute conditions
					for (size_t i = 0; i != N; ++i) {
						a.io_s.post(boost::bind(condition_calls[i].first, boost::cref(a), 
															  boost::ref(success[i]),
															  boost::ref(*this), i));
					}
				}

				void eval_remote_update(const boost::system::error_code&e, 
									    network::actor::remote_proxy<A>* proxy)
				{
					delete proxy;
					check_all_variables_up2date();
				}

				void eval_local_update(const boost::system::error_code&e, int i)
				{
					updating_status[i].terminated = true;
					check_all_variables_up2date();
				}

				template <int N1, typename A1, typename vectorT1, int M1>
				friend struct details::local_update;


			public:
				evaluate_conditions(A& a_, 
									boost::array<condition, N> condition_calls_) :
					a(a_), is_computing(false), condition_calls(condition_calls_) 
				{}

				evaluate_conditions(A& a_, 
									boost::array<condition, N> condition_calls_,
									boost::array<std::string, M> update_status_) :
					a(a_), is_computing(false), condition_calls(condition_calls_) 
				{
					for (size_t i = 0; i < M; ++i)
						updating_status[i].value = update_status_[i];
				}

				evaluate_conditions(A& a_, 
									boost::array<condition, N> condition_calls_,
									boost::array<std::string, M> update_status_,
									const typename remote_values::remote_vars_conf& vars):
					a(a_), is_computing(false), 
					condition_calls(condition_calls_), remote(vars) 
				{
					for (size_t i = 0; i < M; ++i)
						updating_status[i].value = update_status_[i];
				}

				void async_compute(condition_execution_callback cb)
				{
					callbacks.push_back(cb);
					if (is_computing) 
						return;

					/* Reinit the state of the object */
					std::for_each(terminated.begin(), terminated.end(), reset());
					remote.reset();
					for (size_t i = 0; i != M; ++i)
						updating_status[i].terminated = false;

					is_computing = true;

					/* Use a functor template here to be sure that it won't
					 * produce code in M == 0 case. I'm quite sure it does the
					 * right thing in release mode, but it still try to
					 * interpret the code. However, in test case, an agent does
					 * not have a name or an update function (contrary to a
					 * real ability), so use this functor to make sure it
					 * dispatch to the right thing (void if nothing to do))
					 */
					details::local_update<N, A, vectorT, M>(a, *this, updating_status) ();

					if (boost::mpl::size<vectorT>::type::value) {
						network::actor::remote_proxy<A>* proxy = new
							network::actor::remote_proxy<A>(a);

						proxy->async_get(remote, 
								boost::bind(&evaluate_conditions::eval_remote_update,
											this,
											boost::asio::placeholders::error,
											proxy));
					}

					/* 
					 * If M == 0 and vectorT is empty, everything is up2date 
					 * we just need to compute conditions
					 */
					check_all_variables_up2date();
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

		namespace details {
				template <int N, typename A, typename vectorT, int M>
				struct local_update {
					A& a;
					evaluate_conditions<N, A, M, vectorT>& conds;
					boost::array<updating_value, M>& updating_status;

					local_update(A& a, evaluate_conditions<N, A, M, vectorT>& conds, 
								       boost::array<updating_value, M>& updating_status):
						a(a), conds(conds), updating_status(updating_status) {}

					void operator() ()
					{
						/* second and third arguments are not good here, we
						 * need to know in which condition we call this update
						 * condition */
						for (size_t i = 0; i != M; ++i)
							a.updater.async_update(updating_status[i].value, 0, a.name, 
									boost::bind(&evaluate_conditions<N, A, M, vectorT>::eval_local_update,
												&conds,
												boost::asio::placeholders::error,
												i));
					}
				};

				template <int N, typename A, typename vectorT>
				struct local_update<N, A, vectorT, 0> {
					A& a;
					evaluate_conditions<N, A, 0, vectorT>& conds;
					boost::array<updating_value, 0>& updating_status;

					local_update(A& a, evaluate_conditions<N, A, 0, vectorT>& conds, 
								       boost::array<updating_value, 0>& updating_status):
						a(a), conds(conds), updating_status(updating_status) {}

					void operator() () {}
				};
		}
	}
}

#endif /* HYPER_MODEL_EVALUATE_CONDITIONS_HH_ */
