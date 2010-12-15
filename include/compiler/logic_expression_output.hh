#ifndef HYPER_MODEL_LOGIC_EXPRESSION_OUTPUT_HH_
#define HYPER_MODEL_LOGIC_EXPRESSION_OUTPUT_HH_

#include <string>

namespace hyper {
	namespace compiler{
		struct expression_ast;
		std::string generate_logic_expression(const expression_ast& e);
	}
}

#endif /* HYPER_MODEL_LOGIC_EXPRESSION_OUTPUT_HH_ */
