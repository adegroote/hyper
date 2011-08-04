#include <sstream>

#include <logic/facts.hh>
#include <logic/eval.hh>

#include <boost/bind.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace {
	using namespace hyper::logic;

	struct set_inserter : public boost::static_visitor<void>
	{
		facts::sub_expressionV& list;
		facts::sub_expressionS& set;

		set_inserter(facts::sub_expressionV& list_, facts::sub_expressionS& set_) :
			list(list_), set(set_) {}

		template <typename T>
		void operator() (const T& t) const
		{
			set.insert(t);
		}

		void operator() (const function_call& f) const
		{
			set.insert(f);
			set_inserter inserter(list, list[f.id]);
			std::vector<expression>::const_iterator it;
			for (it = f.args.begin(); it != f.args.end(); ++it)
				boost::apply_visitor(inserter, it->expr);
		}
	};

	struct inner_unified_function_call : public boost::static_visitor<void>
	{
		std::vector<function_call> & v;

		inner_unified_function_call(std::vector<function_call>& v) : v(v) {}

		template <typename T>
		void operator() (const T& t) const { (void) t; }

		void operator() (const function_call& f) const 
		{
			v.push_back(f);
		}
	};

	struct inner_function_call : public boost::static_visitor<void>
	{
		const logic_var_db& db;
		std::vector<function_call> & v;

		inner_function_call(const logic_var_db& db, std::vector<function_call>& v_) : 
			db(db), v(v_) {}

		template <typename T>
		void operator() (const T& t) const { (void) t; }

		void operator() (const std::string& sym) const
		{
			const logic_var& var = db.get(sym);
			std::vector<expression>::const_iterator it;
			for (it = var.unified.begin(); it != var.unified.end(); ++it)
				boost::apply_visitor(inner_unified_function_call(v), it->expr);
		}

		void operator() (const function_call& f) const 
		{
			v.push_back(f);
		}
	};

	struct find_predicate {
		const logic_var::identifier_type& id;

		find_predicate(const logic_var::identifier_type& id) : id(id) {}

		bool operator() (const adapt_res::permutation& p) const {
			return p.first == id;
		}
	};

	function_call apply_permutation_f(const function_call& f, const adapt_res::permutationSeq& seq);
	struct apply_permutation_helper : boost::static_visitor<expression>
	{
		const adapt_res::permutationSeq& seq;

		apply_permutation_helper(const adapt_res::permutationSeq& seq) : seq(seq) {}

		template <typename T>
		expression operator() (const T& t) const { return t; }

		expression operator() (const std::string& sym) const 
		{
			adapt_res::permutationSeq::const_iterator it;
			it = std::find_if(seq.begin(), seq.end(), find_predicate(sym));
			if (it != seq.end())
				return it->second;

			return sym;
		}

		expression operator() (const function_call& f) const 
		{
			return apply_permutation_f(f, seq);
		}
	};

	expression apply_permutation_e(const expression& e, const adapt_res::permutationSeq& seq)
	{
		return boost::apply_visitor(apply_permutation_helper(seq), e.expr);
	}

	function_call apply_permutation_f(const function_call& f, const adapt_res::permutationSeq& seq)
	{
		function_call f_res(f);
		for (size_t i = 0; i < f.args.size(); ++i)
			f_res.args[i] = boost::apply_visitor(apply_permutation_helper(seq), f.args[i].expr);

		return f_res;
	}

	struct unified_helper : boost::static_visitor<boost::logic::tribool>
	{
		const logic_var_db& db;
		
		unified_helper(const logic_var_db& db) : db(db) {}

		template <typename T, typename U>
		tribool operator() (const T& t, const U& u) const {
			return indeterminate;
		}

		template <typename U>
		tribool operator() (const Constant<U>& c1, const Constant<U>& c2) const {
			return c1.value == c2.value;
		}

		template <typename U>
		tribool test_unify(const std::string& s, const Constant<U>& c) const {
			const logic_var& v = db.get(s);
			if (v.value)
				return ((*v.value) == c);

			return indeterminate;
		}

		template <typename U>
		tribool operator() (const std::string& s, const Constant<U>& c) const {
			return test_unify(s, c);
		}

		template <typename U>
		tribool operator() (const Constant<U>& c, const std::string& s) const {
			return test_unify(s, c);
		}

		tribool operator() (const std::string& s1, const std::string& s2) const {
			if (s1 == s2) 
				return true;

			const logic_var& v1 = db.get(s1);
			const logic_var& v2 = db.get(s2);

			/* if they are both bound and not to the same value, no chance they unify in the future */
			if (v1.value && v2.value && (*v1.value != *v2.value))
				return false;

			return indeterminate;
		}
	};

	function_call prepare_function_to_exec(const function_call& f, logic_var_db& db);

	struct prepare_exec_helper : public boost::static_visitor<expression>
	{
		logic_var_db& db;
		prepare_exec_helper(logic_var_db& db) : db(db) {}

		template <typename T>
		expression operator() (const T& t) const { return t; }

		expression operator() (const std::string& sym) const {
			const logic_var& v = db.get(sym);
			if (v.value)
				return *v.value;

			return sym;
		}

		expression operator() (const function_call& f) const {
			return prepare_function_to_exec(f, db);
		}
	};
	
	/* Prepare function to exec, replacing bouding symbol by their value */
	function_call prepare_function_to_exec(const function_call& f, logic_var_db& db)
	{
		function_call res(f);
		for (size_t i = 0; i < res.args.size(); ++i)
			res.args[i] = boost::apply_visitor(prepare_exec_helper(db), f.args[i].expr);

		return res;
	}

	tribool eval_f(const function_call& f, const funcDefList& funcs, logic_var_db& db) {
		const function_def& f_def = funcs.get(f.id);
		function_call to_exec(prepare_function_to_exec(db.adapt(f), db));
		return f_def.eval_pred->operator() (to_exec.args);
	}
}

namespace hyper {
	namespace logic {
		struct handle_adapt_res : boost::static_visitor<bool> {

			facts& facts_;
			const function_call& f;

			handle_adapt_res(facts& facts_, const function_call& f) : 
				facts_(facts_), f(f) {}

			bool operator() (const adapt_res::ok& ok) const {
				set_inserter inserter(facts_.sub_list, facts_.sub_list[f.id]);
				boost::apply_visitor(inserter, ok.sym.expr);

				return true; 
			}

			bool operator() (const adapt_res::conflicting_facts& ) const { return false; }

			bool operator() (const function_call& f) const {
				return facts_.add_new_facts(f);
			}

			bool operator() (const adapt_res::require_permutation& perm) const {
				set_inserter inserter(facts_.sub_list, facts_.sub_list[f.id]);
				boost::apply_visitor(inserter, perm.sym.expr);

				return facts_.apply_permutations(perm.seq);
			}
		};

		bool facts::add(const function_call& f)
		{
			resize(f.id, sub_list);
			adapt_res res = db.adapt_and_unify(f);
			return  boost::apply_visitor(handle_adapt_res(*this, f), res.res);
		}

		bool facts::add_new_facts(const function_call& f) 
		{
			resize(f.id, list);
			std::pair< expressionS::iterator, bool> p;
			p = list[f.id].insert(f);
			if (p.second) {
				size__++;
				resize(f.id, sub_list);
				set_inserter inserter(sub_list, sub_list[f.id]);
				std::vector<expression>::const_iterator it;
				for (it = f.args.begin(); it != f.args.end(); ++it)
					boost::apply_visitor(inserter, it->expr);

				/* Insert sub_fact */
				std::vector<function_call> s;
				for (it = f.args.begin(); it != f.args.end(); ++it)
					boost::apply_visitor(inner_function_call(db, s), it->expr);

				bool (facts::*f)(const function_call&) = &facts::add;
				std::for_each(s.begin(), s.end(), boost::bind(f, this, _1));
			}
			return p.second;
		}

		bool facts::apply_permutations(const adapt_res::permutationSeq& seq)
		{
			for (size_t i = 0; i < list.size(); ++i) {
				expressionS tmp;
				std::transform(list[i].begin(), list[i].end(), 
							   std::inserter(tmp, tmp.begin()),
							   boost::bind(&apply_permutation_f, _1,
										 boost::cref(seq)));
				std::swap(list[i], tmp);
			}

			for (size_t i = 0; i < sub_list.size(); ++i) {
				sub_expressionS tmp;	
				std::transform(sub_list[i].begin(), sub_list[i].end(),
							   std::inserter(tmp, tmp.begin()),
							   boost::bind(&apply_permutation_e, _1,
										 boost::cref(seq)));
				std::swap(sub_list[i], tmp);
			}

			return true;
		}

		bool facts::add(const std::string& s)
		{
			generate_return r = hyper::logic::generate(s, funcs);
			assert(r.res);
			if (!r.res) return false;

			return add(r.e);
		}

		boost::logic::tribool facts::matches(const function_call& f) 
		{
			// try to check if the function_call is evaluable
			boost::logic::tribool res;
			res = eval_f(f, funcs, db);

			if (!boost::logic::indeterminate(res)) 
				return res;
			
			// we don't have conclusion from evaluation, check in the know fact
			function_call adapted(db.adapt(f));
			const function_def& def = funcs.get(adapted.id);

			if (def.unify_predicate)
				return boost::apply_visitor(unified_helper(db), adapted.args[0].expr,
																adapted.args[1].expr);
			
			if (adapted.id >= list.size())
				return boost::logic::indeterminate;

			expressionS::const_iterator it = list[adapted.id].begin();
			bool has_matched = false;
			while (!has_matched && it != list[adapted.id].end())
			{
				has_matched = (adapted == *it);
				++it;
			}
			
			if (has_matched) 
				return has_matched;
			return boost::logic::indeterminate;
		}

		function_call facts::generate(const std::string& s)
		{
			generate_return r = hyper::logic::generate(s, funcs);
			assert(r.res);
			return db.adapt(r.e);
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

		struct dump_sublist_facts {
			std::ostream& os;

			dump_sublist_facts(std::ostream& os) : os(os) {}

			void operator() (const facts::sub_expressionS& s) const
			{
				std::copy(s.begin(), s.end(), std::ostream_iterator<expression> (os, ", "));
				os << "\n";
			}
		};

		std::ostream& operator << (std::ostream& os, const facts& f)
		{
			os << f.db << "\n";
			std::for_each(f.list.begin(), f.list.end(), dump_facts(os));
			std::for_each(f.sub_list.begin(), f.sub_list.end(), dump_sublist_facts(os));
			return os;
		}

		struct function_call_to_string {
			std::string operator() (const function_call& f)
			{
				std::ostringstream oss;
				oss << f;
				return oss.str();
			}
		};

		std::vector<function_call> facts::generate(const function_call& f) const
		{
			return  db.deadapt(f);
		}
	}
}
