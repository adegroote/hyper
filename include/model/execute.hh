
#ifndef _HYPER_MODEL_EXECUTION_HH_
#define _HYPER_MODEL_EXECUTION_HH_

#include <map>
#include <string>

#include <logic/expression.hh>
#include <boost/shared_ptr.hpp>

namespace hyper {
	namespace model {
		struct ability;

		template <typename T>
		boost::optional<T> evaluate_expression(const logic::function_call& f, ability &a);

		struct function_execution_base {
			virtual void operator() (const std::vector<logic::expression> & e, 
									ability &a) = 0;
			virtual function_execution_base* clone() = 0;
			virtual boost::any get_result() = 0;
			virtual ~function_execution_base() {};
		};

		class functions_map {
				typedef std::map<std::string, boost::shared_ptr<function_execution_base> > func_map;
				func_map m;

			public:
				void add(const std::string& s, function_execution_base* f)
				{
					m[s] = boost::shared_ptr<function_execution_base>(f);
				}

				function_execution_base* get(const std::string& s)
				{
					return m[s]->clone();
				}
		};
	}
}

#endif /* _HYPER_MODEL_EXECUTION_HH_ */
