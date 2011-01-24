#ifndef HYPER_MODEL_EVALUATE_EXPRESSION_HH_
#define HYPER_MODEL_EVALUATE_EXPRESSION_HH_

#include <model/update_impl.hh>


namespace hyper {
	namespace model {
		namespace details {
			template <typename Fun>
			class evaluate_expression {
				public:
					typedef typename Fun::updater_type updater_type;
					typedef typename Fun::return_type Output;
		
				private:
					updater_type updater;
					boost::optional<Output> res;
		
					template <typename Handler>
					void handle_update(const boost::system::error_code& e,
									   boost::tuple<Handler> handler)
					{
						if (e) {
							boost::get<0>(handler)(e);
							return;
						}
		
						res = Fun()(updater);
						boost::get<0>(handler)(boost::system::error_code());
					}
		
				public:
					evaluate_expression(updater_type updater) : updater(updater) {}
		
					template <typename Handler>
					void async_eval(Handler handler)
					{
						void (evaluate_expression::*f) (
								const boost::system::error_code&e,
								boost::tuple<Handler>) =
							&evaluate_expression::template handle_update<Handler>;
		
						res = boost::none;
						updater.async_update(
								boost::bind(f, this, boost::asio::placeholders::error,
													 boost::make_tuple(handler)));
					}
		
					const boost::optional<Output>& get() const { return res; }
			};
		}

		template <typename Fun>
		class sync_evaluate_expression {
			private:
				typedef typename details::evaluate_expression<Fun> expression_type;
				typedef typename Fun::return_type Output;

				expression_type expression;
				bool has_terminated;
				boost::mutex mtx;
				boost::condition_variable cond;

				void handle_evaluation(const boost::system::error_code& e)
				{
					has_terminated = true;
					boost::lock_guard<boost::mutex> lock(mtx);
					cond.notify_one();
				}

			public:
				sync_evaluate_expression(typename expression_type::updater_type updater):
					expression(updater)
				{}

				Output compute()
				{
					has_terminated = false;
					expression.async_eval(
							boost::bind(&sync_evaluate_expression::handle_evaluation, 
										this,
										boost::asio::placeholders::error));
					{
						boost::unique_lock<boost::mutex> lock(mtx);
						while (!has_terminated) {
							cond.wait(lock);
						}
					}

					const boost::optional<Output>& res = expression.get();
					if (!res) {
						assert(false);
						// the recipe will fail
					} else {
						return *res;
					}

					// Not reacheable
					return Output();
				}
		};
	}
}

#endif /* HYPER_MODEL_EVALUATE_EXPRESSION_HH_ */
