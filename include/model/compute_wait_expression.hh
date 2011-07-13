#ifndef HYPER_WAIT_COMPUTE_WAIT_EXPRESSION_HH_
#define HYPER_WAIT_COMPUTE_WAIT_EXPRESSION_HH_

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>

#include <model/abortable_function.hh>

namespace hyper {
	namespace model {
		class compute_wait_expression : public abortable_function_base {
			private:
				boost::scoped_ptr<abortable_function_base> fun_ptr;
				boost::asio::io_service& io_service_;
				boost::posix_time::time_duration delay_;
				boost::asio::deadline_timer deadline_;
				bool& res;
				bool user_ask_abort;
				bool running;

				typedef boost::function<void (const boost::system::error_code&)> cb_type;

				void handle_timeout(const boost::system::error_code& e, 
									cb_type cb);
				void handle_computation(const boost::system::error_code& e, 
										cb_type cb);

				bool handle_error(const boost::system::error_code& e, cb_type cb);

			public:
				/*
				 * fun_ptr must updated res on invocation !
				 */
				compute_wait_expression(boost::asio::io_service& io_s,
						boost::posix_time::time_duration delay,
						abortable_function_base* fun_ptr,
						bool& res);

				void compute(cb_type cb);
				bool abort();
		};
	}
}

#endif /* HYPER_WAIT_COMPUTE_WAIT_EXPRESSION_HH_ */
