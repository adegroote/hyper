#ifndef HYPER_LOGIC_BACKWARD_CHAINING_HH_
#define HYPER_LOGIC_BACKWARD_CHAINING_HH_

#include <logic/engine.hh>

namespace hyper {
	namespace logic {

		struct backward_chaining {
			const rules& rs;
			facts_ctx& ctx;

			backward_chaining(const rules& rs, facts_ctx& ctx):
				rs(rs), ctx(ctx)
			{}

			bool infer(const function_call& f);
		};
	}
}

#endif /* HYPER_LOGIC_BACKWARD_CHAINING_HH_ */
