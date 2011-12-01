#ifndef HYPER_MODEL_ABORTABLE_FUNCTION_HH_
#define HYPER_MODEL_ABORTABLE_FUNCTION_HH_

#include <vector>

#include <boost/bind.hpp>
#include <boost/function/function0.hpp>
#include <boost/function/function1.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>

#include <logic/expression.hh>

namespace hyper {
	namespace model {

		static inline
		bool none_function() { return false; }

		struct abortable_function_base {
			typedef boost::function<void (const boost::system::error_code& e)> cb_type;
			virtual void compute (cb_type cb) = 0;
			/* Wait for cb or not */
			virtual bool abort() = 0;
			virtual void pause() = 0;
			virtual void resume() = 0;
			virtual logic::expression error() const { return logic::empty(); };
			virtual ~abortable_function_base() {};
		};

		namespace exec_layer_error {
			enum exec_layer_error_t {
				ok,
				interrupted,          // aborted 
				execution_failed,	  // fail to compute remote value, agent dies, ...
				execution_ko		  // compute succesfully, but unexpected error
			};
		}

		class exec_layer_category_impl
			  : public boost::system::error_category
		{
			public:
			  virtual const char* name() const;
			  virtual std::string message(int ev) const;
		};

		const boost::system::error_category& exec_layer_category();

		inline
		boost::system::error_code make_error_code(exec_layer_error::exec_layer_error_t e)
		{
			return boost::system::error_code(static_cast<int>(e), exec_layer_category());
		}


		struct abortable_function : public abortable_function_base {
			typedef boost::function<void (cb_type)> exec_type;
			typedef boost::function<bool (void)> abort_type;

			exec_type exec_;
			abort_type abort_;
			logic::expression error_;
			cb_type cb_;
			bool running;
			bool must_pause;

			abortable_function(exec_type exec, abort_type abort = none_function, 
							   const logic::expression &error = logic::empty());
			void compute (cb_type cb) ;
			bool abort();
			void pause();
			void resume();

			logic::expression error() const { return error_; }

			private:
			void handler(const boost::system::error_code& e, cb_type cb);
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
				ssize_t error_index;

				void handle_computation(const boost::system::error_code& e, size_t index);

				/* Wait for the terminaison of the computation. With ensure
				 * clause, we may have some running clause we need to close
				 * before deleting the task */
				bool wait_terminaison;
				boost::system::error_code err;
				size_t still_pending;
				bool must_pause;

				void terminaison(const boost::system::error_code& e);
				bool check_is_terminated(const boost::system::error_code& e);

			public:
				abortable_computation() : wait_terminaison(false), 
					must_pause(false) {}

				void push_back(abortable_function_base* fun) {
					seq.push_back(abortable_function_ptr(fun));
				}

				void compute(cb_type cb_);
				logic::expression error() const;
				void abort();
				void pause();
				void resume();

				void clear() { seq.clear(); }
				virtual ~abortable_computation() {}
		};
	};
};

namespace boost { namespace system {
	template <>
		struct is_error_code_enum<hyper::model::exec_layer_error::exec_layer_error_t>
		: public true_type {};
}}

#endif /* HYPER_MODEL_ABORTABLE_FUNCTION_HH_ */
