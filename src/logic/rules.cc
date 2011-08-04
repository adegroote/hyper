#include <logic/rules.hh>
#include <algorithm>
#include <boost/variant/apply_visitor.hpp>

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


	struct symbol_adder_ : public boost::static_visitor<void>
	{
		rule::map_symbol& symbols;
		functionId id;

		symbol_adder_(rule::map_symbol & symbols_, functionId id_) : 
			symbols(symbols_), id(id_) {}

		template <typename T>
		void operator() (const T& t) const { (void)t; }

		void operator() (const std::string& t) const
		{
			symbols[t].insert(id);
		}

		void operator() (const function_call& f) const
		{
			std::vector<expression>::const_iterator it;
			// XXX really valid ? 
			for (it = f.args.begin() ; it != f.args.end(); ++it)
				boost::apply_visitor(symbol_adder_(symbols, id), it->expr);
		}

	};

	struct symbol_adder
	{
		rule::map_symbol & symbols;

		symbol_adder(rule::map_symbol& symbols_) : symbols(symbols_) {}

		void operator() (const function_call& f)
		{
			std::vector<expression>::const_iterator it;
			for (it = f.args.begin() ; it != f.args.end(); ++it)
				boost::apply_visitor(symbol_adder_(symbols, f.id), it->expr);
		}

	};

	struct list_keys 
	{
		std::vector<std::string>& v;

		list_keys(std::vector<std::string>& v) : v(v) {}

		void operator() (const std::pair<std::string, std::set<functionId> >& p) 
		{
			v.push_back(p.first);
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

			std::for_each(r.condition.begin(), r.condition.end(), symbol_adder(r.symbol_to_fun));
			std::for_each(r.action.begin(), r.action.end(), symbol_adder(r.symbol_to_fun));

			std::for_each(r.symbol_to_fun.begin(), r.symbol_to_fun.end(), list_keys(r.symbols));

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
