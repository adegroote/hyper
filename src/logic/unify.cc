
#include <logic/unify.hh>

namespace {
	using namespace hyper::logic;

	struct do_unify : public boost::static_visitor< std::pair<bool, unify_p> >
	{
		std::pair<bool, unify_p> cant_unify() const
		{
			return std::make_pair(false, unify_p());
		}

		template <typename T, typename U>
		std::pair<bool, unify_p> can_unify(const T& t, const U& u) const
		{
			return std::make_pair(true, std::make_pair(t, u));
		}

		template <typename T, typename U>
		std::pair<bool, unify_p> operator() (const T& t, const U& u) const
		{
			(void)t; (void) u;
			return cant_unify();
		}

		template <typename T>
		std::pair<bool, unify_p> operator() (const std::string& s, const Constant<T>& c) const
		{
			return can_unify(s, c);
		}

		template <typename T>
		std::pair<bool, unify_p> operator() (const Constant<T>& c, const std::string& s) const
		{
			(void)c; (void) s;
			return cant_unify();
		}
		
		template <typename T, typename U>
		std::pair<bool, unify_p> operator() (const Constant<T>& c1, const Constant<U>& c2) const
		{
			(void) c1; (void) c2;
			return cant_unify();
		}

		template <typename T>
		std::pair<bool, unify_p> operator() (const Constant<T>& c1, const Constant<T>& c2) const
		{
			if (c1.value == c2.value)
				return can_unify(c1, c2);
			return cant_unify();
		}

		std::pair<bool, unify_p> operator() (const std::string& s1, const std::string& s2) const
		{
			return can_unify(s1, s2);
		}


		std::pair<bool, unify_p> operator() (const std::string& s1, const function_call& f1) const
		{
			return can_unify(s1, f1);
		}

		std::pair<bool, unify_p> operator() (const function_call& f, const std::string& s) const
		{
			(void) f; (void) s;
			return cant_unify();
		}
	};
}

namespace hyper {
	namespace logic {
		unify_res unify(const function_call& f1, const function_call& f2)
		{
			if (f1.id != f2.id)
				return std::make_pair(false, unifyV());

			unify_res res;
			res.first = true;
			std::vector<expression>::const_iterator it1 = f1.args.begin();
			std::vector<expression>::const_iterator it2 = f2.args.begin();

			while (res.first && it1 != f1.args.end())
			{
				std::pair<bool, unify_p> local_res;
				local_res = boost::apply_visitor(do_unify(), it1->expr, it2->expr);
				res.first = local_res.first;
				if (res.first)
					res.second.push_back(local_res.second);
				++it1;
				++it2;
			}

			return res;
		}
	}
}
