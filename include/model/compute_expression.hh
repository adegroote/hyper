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
				bool is_updating;
	
				template <typename Handler, typename Ability, typename Local>
				void handle_update(const boost::system::error_code& e,
								   const network::error_context& err_ctx,
								   const Ability& ability,
								   const Local& local,
								   Output& output,
								   boost::tuple<Handler> handler)
				{
					if (e) {
						boost::get<0>(handler)(e);
						return;
					}

					is_updating = false;
					output = Fun()(updater, ability, local);
					boost::get<0>(handler)(boost::system::error_code());
				}
	
			public:
				compute_expression(updater_type updater) : updater(updater) {}
	
				template <typename Handler, typename Ability, typename Local>
				void async_eval(Handler handler, const Ability& a, const Local& local,
								Output& output)
				{
					void (compute_expression::*f) (
							const boost::system::error_code&e,
							const network::error_context&,
							const Ability& ,
							const Local& ,
							Output&,
							boost::tuple<Handler>) =
						&compute_expression::template handle_update<Handler, Ability, Local>;

					is_updating = true;
	
					updater.async_update(
							boost::bind(f, this, boost::asio::placeholders::error,
												 _2, 
												 boost::cref(a), 
												 boost::cref(local),
												 boost::ref(output),
												 boost::make_tuple(handler)));
				}

				hyper::network::runtime_failure error() const 
				{ 
					hyper::network::runtime_failure f = 
						hyper::network::execution_failure( 
								Fun::logic_expression() 
								);
					if (is_updating) 
						f.error_cause = updater.error();
					return f;
				}
        };
    }
}

#endif /* HYPER_MODEL_COMPUTE_EXPRESSION_HH_ */
