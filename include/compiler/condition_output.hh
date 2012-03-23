#ifndef HYPER_MODEL_CONDITION_OUTPUT_HH_
#define HYPER_MODEL_CONDITION_OUTPUT_HH_

#include <iostream>

#include <boost/optional.hpp>

namespace hyper {
	namespace compiler {
		struct expression_ast; 
		class ability;
		class universe;

		struct generate_condition {
			std::ostream& oss;
			std::string base;
			const ability& a;
			boost::optional<const universe&> u;
			mutable size_t counter;


			generate_condition(std::ostream& oss_, const std::string& base_,
							   const ability& a, 
							   boost::optional<const universe&> u = boost::none) : 
				oss(oss_), base(base_), a(a), u(u), counter(0){}

			void operator() (const expression_ast& e) const;
		};
	}
}

#endif /* HYPER_MODEL_CONDITION_OUTPUT_HH_ */
