#include <logic/function_def.hh>
#include <logic/eval.hh>

namespace hyper {
	namespace logic {
		funcDefList::funcDefList()
		{
			list_eval.push_back(new eval<notAPredicate, 0>());
		}

		struct Delete 
		{ 
			template <class T> void operator ()(T*& p) const 
			{ 
				delete p;
				p = NULL;
			} 
		}; 

		funcDefList::~funcDefList()
		{
			std::for_each(list_eval.begin(), list_eval.end(), Delete());
		}

		functionId funcDefList::add(const std::string& name, size_t arity, eval_predicate* eval)
		{
			funcM::const_iterator it = m.find(name);
			if (it == m.end()) 
			{
				if (eval == 0) 
					list.push_back(function_def(name, arity, list_eval[0]));
				else {
					// get ownership
					list_eval.push_back(eval);
					list.push_back(function_def(name, arity, eval));
				}
				m[name] = list.size() - 1;
				return list.size() -1;
			} else 
				return it->second;

			return 0; /* Not reachable */
		}


		boost::optional<function_def> funcDefList::get(const std::string& name) const
		{
			funcM::const_iterator it = m.find(name);
			if (it == m.end())
				return boost::none;
			else {
				function_def def = get(it->second);
				return def;
			}

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
