#include <logic/facts.hh>
#include <logic/eval.hh>

namespace hyper {
	namespace logic {
		bool facts::add(const function_call& f)
		{
			if (f.id >= list.size())
				list.resize(funcs.size());
			std::pair< expressionS::iterator, bool> p;
			p = list[f.id].insert(f);
			if (p.second)
				size__++;
			return p.second;
		}

		bool facts::add(const std::string& s)
		{
			generate_return r = generate(s, funcs);
			assert(r.res);
			if (!r.res) return false;

			return add(r.e);
		}

		bool facts::matches(const function_call& f) const
		{
			// try to check if the function_call is evaluable
			boost::logic::tribool res;
			const function_def& f_def = funcs.get(f.id);
			res = f_def.eval_pred->operator() (f.args);

			if (!boost::logic::indeterminate(res))
				return res;

			// we don't have conclusion from evaluation, check in the know fact
			
			if (f.id >= list.size())
				return false;

			expressionS::const_iterator it = list[f.id].begin();
			bool has_matched = false;
			while (!has_matched && it != list[f.id].end())
			{
				has_matched = (f == *it);
				++it;
			}
			
			return has_matched;
		}

		struct dump_facts
		{
			std::ostream& os;

			dump_facts(std::ostream& os_) : os(os_) {}

			void operator() (const facts::expressionS& s) const
			{
				std::copy(s.begin(), s.end(), std::ostream_iterator<expression> ( os, "\n"));
			}
		};

		std::ostream& operator << (std::ostream& os, const facts& f)
		{
			std::for_each(f.list.begin(), f.list.end(), dump_facts(os));
			return os;
		}
	}
}
