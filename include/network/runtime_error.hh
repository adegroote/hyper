#ifndef HYPER_NETWORK_RUNTIME_ERROR_HH_
#define HYPER_NETWORK_RUNTIME_ERROR_HH_

#include <logic/expression.hh>

#include <boost/variant/variant.hpp>


namespace hyper {
	namespace network {

		struct success {};

		struct assertion_failure {
			logic::expression what;

			assertion_failure() {}
			explicit assertion_failure(const logic::expression& what_):
				what(what_) {}
		};

		struct constraint_failure {
			logic::expression what;

			constraint_failure() {}
			explicit constraint_failure(const logic::expression& what_):
				what(what_) {}
		};

		struct read_failure {
			std::string symbol;

			read_failure() {}
			explicit read_failure(const std::string& symbol_):
				symbol(symbol_) {}
		};

		struct execution_failure {
			logic::expression what;

			execution_failure() {}
			explicit execution_failure(const logic::expression& what_):
				what(what_) {}
		};

		struct runtime_failure {
			typedef
				boost::variant<
				  success 
				, assertion_failure
				, constraint_failure
				, read_failure
				, execution_failure
				>
				type;

			runtime_failure()
				: error(success()) {}

			template <typename Error>
			runtime_failure(Error const& error)
				: error(error) {}

			type error;
			std::string recipe_name;

			std::vector<runtime_failure> error_cause;
		};


		std::ostream& operator<< (std::ostream& os, const runtime_failure& r);

	}
}


#endif /* HYPER_NETWORK_RUNTIME_ERROR_HH_ */
