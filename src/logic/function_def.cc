#include <logic/function_def.hh>

namespace hyper {
	namespace logic {
		functionId funcDefList::add(const std::string& name, size_t arity)
		{
			funcM::const_iterator it = m.find(name);
			if (it == m.end()) 
			{
				list.push_back(function_def(name, arity));
				m[name] = list.size() - 1;
				return list.size() -1;
			} else 
				return it->second;

			return 0; /* Not reachable */
		}

		function_def funcDefList::get(functionId id) const
		{
			assert(id < list.size());
			return list[id];
		}

		boost::optional<function_def> funcDefList::get(const std::string& name) const
		{
			funcM::const_iterator it = m.find(name);
			if (it == m.end())
				return boost::none;
			else
				return get(it->second);

			return boost::none;
		}

		boost::optional<functionId> funcDefList::getId(const std::string& name) const
		{
			funcM::const_iterator it = m.find(name);
			if (it == m.end())
				return boost::none;
			else
				return it->second;

			return boost::none;
		}
	}
}
