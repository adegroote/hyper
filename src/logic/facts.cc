#include <logic/facts.hh>

namespace hyper {
	namespace logic {
		bool facts::matches(const expression& e) const
		{
			expressionV::const_iterator it = list.begin();
			bool has_matched = false;
			while (!has_matched && it != list.end())
			{
				has_matched = (e == *it);
				++it;
			}
			
			return has_matched;
		}
	}
}
