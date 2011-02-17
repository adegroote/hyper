#ifndef HYPER_MODEL_COMPUTE_EXPRESSION_HH_
#define HYPER_MODEL_COMPUTE_EXPRESSION_HH_

#include <model/update_impl.hh>


namespace hyper {
	namespace model {
		template <typename Fun>
		class compute_expression {
			public:
				typedef typename Fun::updater_type updater_type;
				typedef typename Fun::return_type Output;
	
			private:
				updater_type updater;
	
				template <typename Handler>
				void handle_update(const boost::system::error_code& e,
								   Output& output,
								   boost::tuple<Handler> handler)
				{
					if (e) {
						boost::get<0>(handler)(e);
						return;
					}
	
					output = Fun()(updater);
					boost::get<0>(handler)(boost::system::error_code());
				}
	
			public:
				compute_expression(updater_type updater) : updater(updater) {}
	
				template <typename Handler>
				void async_eval(Handler handler, Output& output)
				{
					void (compute_expression::*f) (
							const boost::system::error_code&e,
							Output&,
							boost::tuple<Handler>) =
						&compute_expression::template handle_update<Handler>;
	
					updater.async_update(
							boost::bind(f, this, boost::asio::placeholders::error,
												 boost::ref(output),
												 boost::make_tuple(handler)));
				}
		};
	}
}

#endif /* HYPER_MODEL_COMPUTE_EXPRESSION_HH_ */
