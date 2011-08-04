
#ifndef _HYPER_MODEL_EXECUTION_HH_
#define _HYPER_MODEL_EXECUTION_HH_

#include <map>
#include <string>

#include <logic/expression.hh>

#include <boost/any.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/function/function2.hpp>
#include <boost/shared_ptr.hpp>

namespace hyper {
	namespace model {
		struct ability;

		struct function_execution_base {
			virtual void async_compute(
					boost::asio::io_service& io_s,
					const std::vector<logic::expression> &e,
					ability &a,
					boost::function<void (const boost::system::error_code&)> cb) = 0;

			virtual function_execution_base* clone() = 0;
			virtual boost::any get_result() = 0;
			virtual bool is_finished() const = 0;
			virtual ~function_execution_base() {};
		};

		template <typename T>
		boost::optional<T> evaluate_expression(
				boost::asio::io_service& io_s, 
				const logic::function_call& f, ability &a);

		template <typename T, typename Handler>
		void async_eval_expression(
			 boost::asio::io_service& io_s,
			 const logic::function_call& f,
			 ability& a,
			 boost::optional<T> & res,
			 Handler handler
		);

		class functions_map {
				typedef std::map<std::string, boost::shared_ptr<function_execution_base> > func_map;
				func_map m;

			public:
				void add(const std::string& s, function_execution_base* f);
				function_execution_base* get(const std::string& s) const;
		};
	}
}

#endif /* _HYPER_MODEL_EXECUTION_HH_ */
