#include <logic/rules.hh>


namespace {
	using namespace hyper::logic;

	struct converter {
		bool& res;
		const funcDefList& funcs;
		std::vector<function_call>& res_vector;

		converter(bool& res_, const funcDefList& funcs_,
				  std::vector<function_call>& res_vector_):
			res(res_), funcs(funcs_), res_vector(res_vector_)
		{}

		void operator() (const std::string& s) 
		{
			if (!res) return;
			generate_return r = generate(s, funcs);
			res = r.res;
			if (res)
				res_vector.push_back(r.e);
		}
	};
}
namespace hyper {
	namespace logic {
		std::ostream& operator << (std::ostream& os, const rule& r)
		{
			os << r.identifier << " : \n";
			os << "IF ";
			std::copy(r.condition.begin(), r.condition.end(),
					      std::ostream_iterator<function_call> (os, " && " ));
			os << "true \nTHEN ";
			std::copy(r.action.begin(), r.action.end(),
					      std::ostream_iterator<function_call> (os, " && " ));
			os << "true";
			return os;
		}

		bool rules::add(const std::string& identifier,
						const std::vector<std::string>& cond,
						const std::vector<std::string>& action)
		{
			rule r;
			{
			bool res = true;
			converter c(res, funcs, r.condition);
			std::for_each(cond.begin(), cond.end(), c);
			if (!res) return false;
			}

			{
			bool res = true;
			converter c(res, funcs, r.action);
			std::for_each(action.begin(), action.end(), c);
			if (!res) return false;
			}
			r.identifier = identifier;

			r_.push_back(r);
			return true;
		}

		std::ostream& operator << (std::ostream& os, const rules& r)
		{
			std::copy(r.begin(), r.end(), std::ostream_iterator<rule> ( os, "\n"));
			return os;
		}
	}
}
