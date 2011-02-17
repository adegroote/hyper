#ifndef HYPER_MODEL_ABORTABLE_FUNCTION_HH_
#define HYPER_MODEL_ABORTABLE_FUNCTION_HH_

#include <vector>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace hyper {
	namespace model {

		static inline
		void none_function() {}

		struct abortable_function_base {
			typedef boost::function<void (const boost::system::error_code& e)> cb_type;
			virtual void compute (cb_type cb) = 0;
			virtual void abort() = 0 ;
			virtual ~abortable_function_base() {};
		};

		struct abortable_function : public abortable_function_base {
			typedef boost::function<void (cb_type)> exec_type;
			typedef boost::function<void (void)> abort_type;

			exec_type exec_;
			abort_type abort_;

			abortable_function(exec_type exec, abort_type abort = none_function):
				exec_(exec), abort_(abort) {}

			void compute (cb_type cb) {
				return exec_(cb);
			}

			void abort() {
				return abort_();
			}
		};

		/* 
		 * Abortable_computation represents a sequence of function, chained by
		 * a continuation model (a continuation monad in other term).
		 *
		 * The computation will fail if any element of the chain fails. 
		 * The computation may be aborted by the user, in this case, the
		 * callback will return an interrupted error_code
		 */

		class abortable_computation {
			public:
				typedef boost::function<void (const boost::system::error_code& e)> cb_type;

			private:
				typedef boost::shared_ptr<abortable_function_base> abortable_function_ptr;
				typedef std::vector<abortable_function_ptr> computation_seq;


				computation_seq seq;
				cb_type cb;
				size_t index;
				bool user_ask_abort;

				void handle_computation(const boost::system::error_code& e)
				{
					if (e) 
						cb(e);
					else {
						if (user_ask_abort) 
							return cb(make_error_code(boost::system::errc::interrupted));
						if (index+1 == seq.size())
							cb(boost::system::error_code());
						else {
							index++;
							seq[index]->compute(boost::bind(
										&abortable_computation::handle_computation,
										this, boost::asio::placeholders::error));
						}
					}
				}

			public:
				abortable_computation() {}

				void push_back(abortable_function_base* fun) {
					seq.push_back(abortable_function_ptr(fun));
				}

				void compute(cb_type cb_) {
					cb = cb_;
					index = 0;
					user_ask_abort = false;

					seq[index]->compute(boost::bind(&abortable_computation::handle_computation,
										this, boost::asio::placeholders::error));
				}

				void abort() {
					seq[index]->abort();
					user_ask_abort = true;
				}

				void clear() { seq.clear(); }
				virtual ~abortable_computation() {};
		};
	};
};

#endif /* HYPER_MODEL_ABORTABLE_FUNCTION_HH_ */
