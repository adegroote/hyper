#ifndef HYPER_NETWORK_RUNTIME_ERROR_HH_
#define HYPER_NETWORK_RUNTIME_ERROR_HH_

#include <logic/expression.hh>

#include <boost/variant/variant.hpp>


namespace hyper {
	namespace network {


		struct unknown_error {};

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
			std::string extra_information;

			execution_failure() {}
			explicit execution_failure(const logic::expression& what_):
				what(what_), extra_information("") {}

			execution_failure(const logic::expression& what_,
				const std::string& extra_information_):
				what(what_), extra_information(extra_information_)
			{}
		};

		struct runtime_failure {
			typedef
				boost::variant<
				  unknown_error 
				, assertion_failure
				, constraint_failure
				, read_failure
				, execution_failure
				>
				type;

			runtime_failure()
				: error(unknown_error()) {}

			template <typename Error>
			runtime_failure(Error const& error)
				: error(error) {}

			type error;

			std::string agent_name;
			std::string task_name;
			std::string recipe_name;

			std::vector<runtime_failure> error_cause;

			void pretty_print(std::ostream& os, int indent = 0) const;
		};


		std::ostream& operator<< (std::ostream& os, const runtime_failure& r);

		static const runtime_failure default_error = network::runtime_failure();

		typedef std::vector<network::runtime_failure> error_context;
		typedef std::vector<network::runtime_failure>::iterator error_context_iterator;
		typedef std::vector<network::runtime_failure>::const_iterator error_context_const_iterator;
		static const error_context empty_error_context = std::vector<network::runtime_failure>();

		std::ostream& operator<< (std::ostream& os, const error_context& err);
	}
}


#endif /* HYPER_NETWORK_RUNTIME_ERROR_HH_ */
