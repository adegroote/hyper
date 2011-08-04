#include <logic/unify.hh>
#include <boost/variant/apply_visitor.hpp>

namespace {
	using namespace hyper::logic;

	struct do_unify : public boost::static_visitor<bool>
	{
		unifyM& res;

		do_unify(unifyM& res_) : res(res_) {}

		bool cant_unify() const
		{
			return false;
		}

		template <typename U>
		bool can_unify(const std::string& t, const U& u) const
		{
			unifyM::const_iterator it = res.find(t);
			/* 
			 * if the variable is already unified, check if the unification
			 * leads to the same expression. If it is not the case, the
			 * unification is not valid
			 */
			if (it != res.end()) 
			{
				return (it->second == u); 
			}
			res[t] = u;
			return true;
		}

		template <typename T, typename U>
		bool operator() (const T& t, const U& u) const
		{
			(void)t; (void) u;
			return cant_unify();
		}

		template <typename T>
		bool operator() (const std::string& s, const Constant<T>& c) const
		{
			return can_unify(s, c);
		}

		template <typename T>
		bool operator() (const Constant<T>& c, const std::string& s) const
		{
			(void)c; (void) s;
			return cant_unify();
		}
		
		template <typename T, typename U>
		bool operator() (const Constant<T>& c1, const Constant<U>& c2) const
		{
			(void) c1; (void) c2;
			return cant_unify();
		}

		template <typename T>
		bool operator() (const Constant<T>& c1, const Constant<T>& c2) const
		{
			return (c1.value == c2.value);
		}

		bool operator() (const std::string& s1, const std::string& s2) const
		{
			// XXX wrong
			return can_unify(s1, s2);
		}


		bool operator() (const std::string& s1, const function_call& f1) const
		{
			return can_unify(s1, f1);
		}

		bool operator() (const function_call& f, const std::string& s) const
		{
			(void) f; (void) s;
			return cant_unify();
		}
	};
}

namespace hyper {
	namespace logic {
		unify_res unify(const function_call& f1, const function_call& f2, const unifyM& ctx)
		{
			if (f1.id != f2.id)
				return std::make_pair(false, unifyM());

			unify_res res;
			res.first = true;
			res.second = ctx; // initialize it with current context 

			std::vector<expression>::const_iterator it1 = f1.args.begin();
			std::vector<expression>::const_iterator it2 = f2.args.begin();

			while (res.first && it1 != f1.args.end())
			{
				res.first = boost::apply_visitor(do_unify(res.second), it1->expr, it2->expr);
				++it1;
				++it2;
			}

			return res;
		}

		std::ostream& operator << (std::ostream& os, const unifyM& m)
		{
			for (unifyM::const_iterator it = m.begin(); it != m.end(); ++it)
				os << "\tunifying " << it->first << " with " << it-> second << "\n";
			return os;
		}
	}
}
