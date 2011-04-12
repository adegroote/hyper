#ifndef HYPER_MODEL_LOGIC_EXPRESSION_OUTPUT_HH_
#define HYPER_MODEL_LOGIC_EXPRESSION_OUTPUT_HH_

#include <string>
#include <boost/optional.hpp>

namespace hyper {
	namespace compiler{
		struct expression_ast;
		class ability;
		class universe;

		std::string generate_logic_expression(const expression_ast& e, 
				const ability& a, boost::optional<const universe&> u = boost::none);
	}
}

#endif /* HYPER_MODEL_LOGIC_EXPRESSION_OUTPUT_HH_ */
