#ifndef HYPER_MODEL_CONDITION_OUTPUT_HH_
#define HYPER_MODEL_CONDITION_OUTPUT_HH_

#include <iostream>

namespace hyper {
	namespace compiler {
		struct expression_ast; 

		struct generate_condition {
			std::ostream& oss;
			size_t counter;

			generate_condition(std::ostream& oss_) : oss(oss_), counter(0) {}

			void operator() (const expression_ast& e) const;
		};
	}
}

#endif /* HYPER_MODEL_CONDITION_OUTPUT_HH_ */
