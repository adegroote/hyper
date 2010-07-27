#include <logic/facts.hh>
#include <logic/eval.hh>

namespace hyper {
	namespace logic {
		bool facts::matches(const function_call& f) const
		{
			// try to check if the function_call is evaluable
			boost::logic::tribool res;
			const function_def& f_def = funcs.get(f.id);
			res = f_def.eval_pred->operator() (f.args);

			if (!boost::logic::indeterminate(res))
				return res;

			// we don't have conclusion from evaluation, check in the know fact
			expressionV::const_iterator it = list.begin();
			bool has_matched = false;
			while (!has_matched && it != list.end())
			{
				has_matched = (f == *it);
				++it;
			}
			
			return has_matched;
		}
	}
}
