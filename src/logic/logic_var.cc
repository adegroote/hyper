#include <sstream>
#include <iostream>
#include <set>

#include <logic/logic_var.hh>
#include <utils/algorithm.hh>

#include <boost/bind.hpp>
#include <boost/variant/apply_visitor.hpp>

static int generator;


namespace {
	using namespace hyper::logic;
	logic_var::identifier_type new_identifier() 
	{
		std::ostringstream oss;
		oss << "__L" << generator++;
		return oss.str();
	}

	bool is_logic_var(const std::string& s)
	{
		return (s.size() > 3 && s[0] == '_' 
							 && s[1] == '_'
							 && s[2] == 'L');
	}

	struct replace_logic_var_helper : public boost::static_visitor<expression>
	{
		logic_var::identifier_type v1, v2;

		replace_logic_var_helper(const logic_var::identifier_type& v1,
								 const logic_var::identifier_type& v2):
			v1(v1), v2(v2)
		{}

		template <typename T>
		expression operator() (const T& t) const { return t; }

		expression operator() (const std::string& sym) const {
			if (sym == v2) 
				return v1;

			return sym;
		}

		expression operator() (const function_call& f) const {
			function_call f_res(f);
			for (size_t i = 0; i < f.args.size(); ++i) 
				f_res.args[i] = boost::apply_visitor(*this, f.args[i].expr);

			return f_res;
		}
	};

	expression replace_logic_var(const expression& e, const logic_var::identifier_type& v1,
												      const logic_var::identifier_type& v2)
	{
		return boost::apply_visitor(replace_logic_var_helper(v1, v2), e.expr);
	}
									
	function_call adapt_function_call(logic_var_db::bm_type& bm, 
									  logic_var_db::map_type& m,
									  std::vector<logic_var::identifier_type>& v,
									  const function_call& f);

	struct adapt_function_call_helper : public boost::static_visitor<expression>
	{
		typedef logic_var_db::bm_type bm_type;
		typedef logic_var_db::value_type value_type; 

		logic_var_db::bm_type& bm;
		logic_var_db::map_type& m;
		std::vector<logic_var::identifier_type>& v;

		adapt_function_call_helper(logic_var_db::bm_type& bm,
								   logic_var_db::map_type& m,
								   std::vector<logic_var::identifier_type>& v):
			bm(bm), m(m), v(v)
		{}

		template <typename T>
		expression operator() (const T& t) const { return t; }

		expression operator() (const std::string& sym) const {
			/* Check it is not already a logic variable */
			logic_var_db::bm_type::left_iterator it1 = bm.left.find(sym);
			if (it1 != bm.left.end())
				return sym;

			logic_var_db::bm_type::right_iterator it = bm.right.find(sym);
			if (it == bm.right.end()) {
				logic_var::identifier_type new_id = new_identifier();
				logic_var var(new_id);
				var.unified.push_back(sym);

				bm.insert(value_type(new_id, sym));
				m.insert(std::make_pair(new_id, var));
				v.push_back(new_id);
				return new_id;
			} else {
				return it->second;
			}
		};

		expression operator() (const function_call& f) const {
			expression e = adapt_function_call(bm, m, v, f);
			logic_var_db::bm_type::right_iterator it = bm.right.find(e);
			if (it == bm.right.end()) {
				logic_var::identifier_type new_id = new_identifier();
				logic_var var(new_id);
				var.unified.push_back(e);

				bm.insert(value_type(new_id, e));
				m.insert(std::make_pair(new_id, var));
				v.push_back(new_id);
				return new_id;
			} else {
				return it->second;
			}
		}
	};

	function_call adapt_function_call(logic_var_db::bm_type& bm, 
									  logic_var_db::map_type& m,
									  std::vector<logic_var::identifier_type>& v,
									  const function_call& f)
	{
		function_call f_res(f);
		for (size_t i = 0; i < f_res.args.size(); ++i) 
			f_res.args[i] = boost::apply_visitor(adapt_function_call_helper(bm, m, v), f_res.args[i].expr);

		return f_res;
	}

	expression apply_permutation_on_facts(const expression& e_, 
										  const adapt_res::permutationSeq& perms) 
	{
		expression e(e_);
		adapt_res::permutationSeq::const_iterator p;
		for (p = perms.begin(); p != perms.end(); ++p)
			e = replace_logic_var(e, p->second, p->first);
		return e;
	}

	struct find_predicate {
		const logic_var::identifier_type& id;

		find_predicate(const logic_var::identifier_type& id) : id(id) {}

		bool operator() (const adapt_res::permutation& p) const {
			return p.first == id;
		}
	};

	struct apply_perms {
		const adapt_res::permutationSeq& perms;

		apply_perms(const adapt_res::permutationSeq& perms) : perms(perms) {}

		void operator() (std::pair<const logic_var::identifier_type,
				logic_var> & p) const
		{
			std::vector<expression> s(p.second.unified.size());
			std::transform(p.second.unified.begin(), p.second.unified.end(),
					s.begin(),
					boost::bind(&apply_permutation_on_facts, _1,
						boost::cref(perms)));

			std::swap(p.second.unified, s);
		}
	};

	struct unify_helper : public boost::static_visitor<bool>
	{
		logic_var_db::bm_type& bm;
		logic_var_db::map_type& m;
		adapt_res::permutationSeq& seq;

		unify_helper(logic_var_db::bm_type& bm, logic_var_db::map_type& m,
					 adapt_res::permutationSeq& seq) : 
			bm(bm), m(m), seq(seq) {}

		template <typename U>
		bool do_unify(const std::string& s, const Constant<U>& u) const
		{
			logic_var_db::map_type::iterator it = m.find(s);
			assert (it != m.end());

			// if already bounded, check it will leads to the same thing
			if (it->second.value) 
				return (*(it->second.value) == u);
	
			it->second.value = u;
			it->second.unified.push_back(u);
			return true;
		}

		template <typename U, typename V>
		bool operator() ( const U&, const V&) const
		{
			return false;
		}

		template <typename U>
		bool operator() (const Constant<U>& u, const Constant<U>& v) const
		{
			return (u.value == v.value);
		}

		template <typename U>
		bool operator() (const Constant<U>& u, const std::string& s) const
		{
			return do_unify(s, u);
		}

		template <typename U>
		bool operator() (const std::string& s, const Constant<U>& u) const
		{
			return do_unify(s, u);
		}

		void apply_permutations(std::vector<adapt_res::permutation>& perms) const
		{
			logic_var_db::bm_type::right_iterator it;
			for (it = bm.right.begin(); it != bm.right.end(); ++it) {
				expression e = apply_permutation_on_facts(it->first, perms);
				bool success = bm.right.replace_key(it, e);
				// it means that the same fact appears twice, so we need to aggregate them
				if (!success) {
					logic_var_db::bm_type::right_iterator it2 = bm.right.find(e);
					perms.push_back(std::make_pair(it->second, it2->second));
					m.erase(it->second);
					bm.right.erase(it);
					return apply_permutations(perms);
				}
			}
		}

		bool operator() (const std::string& s1, const std::string& s2) const
		{
			logic_var_db::map_type::iterator it1, it2;
			it1 = m.find(s1);
			it2 = m.find(s2);
			assert (it1 != m.end() && it2 != m.end() );

			if (it1 == it2)
				return true;

			logic_var var(it1->second);
			// they both have values, check that they are the same
			if (var.value && it2->second.value) {
				if (! (*var.value == *(it2->second.value)))
					return false;
			}

			if (!var.value && it2->second.value) 
				var.value = it2->second.value;
			std::copy(it2->second.unified.begin(), it2->second.unified.end(),
					  std::back_inserter(var.unified));

			m.erase(it2);
			it1->second = var;

			seq.push_back(std::make_pair(s2, s1));
			apply_permutations(seq);

			logic_var_db::bm_type::left_iterator it = bm.left.begin();
			while (it != bm.left.end()) {
				adapt_res::permutationSeq::const_iterator it2;
				it2 = std::find_if(seq.begin(), seq.end(), find_predicate(it->first));
				if (it2 != seq.end()) {
					bm.left.replace_key(it, it2->second);
					it = bm.left.begin();
				} else {
					++it;
				}
			}

			// update facts stored in logic_var with the permutations
			std::for_each(m.begin(), m.end(), apply_perms(seq));
			return true;
		}
	};

	bool unify(logic_var_db::bm_type& bm, logic_var_db::map_type& m, 
			   const expression& e1, const expression& e2,
			   adapt_res::permutationSeq& seq)
	{
		return boost::apply_visitor(unify_helper(bm, m, seq), e1.expr, e2.expr);
	}

	struct are_newly_introduced {
		const std::vector<logic_var::identifier_type> &v;

		are_newly_introduced(const std::vector<logic_var::identifier_type>& v) :
			v(v)
		{}

		bool operator () (const adapt_res::permutation& p) const
		{
			std::vector<logic_var::identifier_type>::const_iterator it;
			it = std::find(v.begin(), v.end(), p.first);
			return (it != v.end());
		}
	};


	void extract_logic_var(const function_call& f, std::set<std::string>& s);

	struct extract_logic_var_helper : boost::static_visitor<void>
	{
		std::set<std::string>& s;

		extract_logic_var_helper(std::set<std::string>& s) : s(s) {}

		template <typename T>
		void operator() (const T& ) const {}

		void operator() (const std::string& sym) const { 
			if (is_logic_var(sym)) s.insert(sym);
		}

		void operator() (const function_call& f) const { extract_logic_var(f, s); }
	};


	void extract_logic_var(const function_call& f, std::set<std::string>& s)
	{
		for (size_t i = 0; i < f.args.size(); ++i)
			boost::apply_visitor(extract_logic_var_helper(s), f.args[i].expr);
	}
		
	typedef std::map<std::string, std::vector<expression> > logic_var_to_exprM;
	typedef std::map<std::string, expression> logic_mappingM;

	struct generate_funcall
	{
		std::vector<std::string> v_sym;
		const logic_var_to_exprM& map;

		struct iter {
			std::vector<expression>::const_iterator begin;
			std::vector<expression>::const_iterator end;
			std::vector<expression>::const_iterator current;
		};

		std::vector<iter> v;

		generate_funcall(const std::set<std::string>& sym,
						 const logic_var_to_exprM& map) : v_sym(sym.begin(), sym.end()), map(map) 
		{
			for (size_t i = 0; i < v_sym.size(); ++i) {
				logic_var_to_exprM::const_iterator it = map.find(v_sym[i]);
				assert(it != map.end());
				iter current;
				current.begin = it->second.begin();
				current.end = it->second.end();
				current.current = current.begin;
				v.push_back(current);
			}
		}

		/* 
		 * Compute the next possible combinaison, returns false if it is the
		 * last one */
		bool next(logic_mappingM &m)
		{
			// generate the current solution
			for (size_t i = 0; i < v_sym.size(); ++i)
				m[v_sym[i]] = *v[i].current;

			// next one
			size_t i = 0;
			bool add = true;

			while (add && i < v_sym.size()) {
				++v[i].current;
				if (v[i].current == v[i].end) {
					v[i].current = v[i].begin;
					++i;
					add = true;
				} else {
					add = false;
				}
			}

			return !add;
		}
	};

	function_call replace(const function_call& f, const logic_mappingM& m);

	struct replace_helper : public boost::static_visitor<expression>
	{
		const logic_mappingM& m;

		replace_helper(const logic_mappingM& m) : m(m) {}

		template <typename T>
	    expression operator() (const T& t) const { return t; }

		expression operator() (const std::string& sym) const {
			logic_mappingM::const_iterator it = m.find(sym);
			assert(it != m.end());
			return it->second;
		}

		expression operator() (const function_call& f) const {
			return replace(f, m);
		}
	};

	function_call replace(const function_call& f, const logic_mappingM& m)
	{
		function_call f_res(f);
		for (size_t i = 0; i < f_res.args.size(); ++i)
			f_res.args[i] = boost::apply_visitor(replace_helper(m), f.args[i].expr);
		return f_res;
	}

	expression replace(const expression& e, const logic_mappingM& m)
	{
		return boost::apply_visitor(replace_helper(m), e.expr);
	}

	struct generate_mapping {
		const logic_var_db& db;
		logic_var_to_exprM& m;

		generate_mapping(const logic_var_db& db,
						 logic_var_to_exprM& m):
			db(db), m(m) 
		{}

		void operator() (const std::string& sym) {
			// we already generate the mapping for this symbol
			if (m.find(sym) != m.end()) 
				return;

			const logic_var& var = db.get(sym);
			std::vector<expression> v;
			for (size_t i = 0; i < var.unified.size(); ++i) {
				std::set<std::string> s;
				boost::apply_visitor(extract_logic_var_helper(s), var.unified[i].expr);
				if (s.empty()) 
					v.push_back(var.unified[i]);
				else {
					// if we have loop, we have a problem !!! :D
					std::for_each(s.begin(), s.end(), generate_mapping(db, m));
					logic_mappingM local_mapping;
					generate_funcall gen(s, m);
					bool has_next = true;
					while (has_next) {
						has_next = gen.next(local_mapping);
						v.push_back(replace(var.unified[i], local_mapping));
					}
				}
			}

			m[sym] = v;
		}
	};
}

namespace hyper { namespace logic {

std::ostream& operator << (std::ostream& os, const logic_var& l)
{
	os << "logic variable " << l.id;
	os << " with value ";
	if (l.value) 
		os << *(l.value);
	else
		os << " none ";

	os << " linked with real vars [ ";
	std::copy(l.unified.begin(), l.unified.end(), std::ostream_iterator<expression>(os, ", "));
	os << " ]";
	return os;
}

std::ostream& operator<<(std::ostream& os, const logic_var_db& db)
{
	logic_var_db::bm_type::left_const_iterator it;
	for (it = db.bm.left.begin(); it != db.bm.left.end(); ++it)
		os  << it->first << " <==> " << it->second << "\n"; 

	logic_var_db::map_type::const_iterator it1;
	for (it1 = db.m_logic_var.begin(); it1 != db.m_logic_var.end(); ++it1)
		os << it1->second << "\n";

	return os;
}

function_call
logic_var_db::adapt(const function_call& f)
{
	std::vector<logic_var::identifier_type> v;
	return adapt_function_call(bm, m_logic_var, v, f);
}

adapt_res 
logic_var_db::adapt_and_unify(const function_call& f)
{
	// track newly introduced logic variable
	std::vector<logic_var::identifier_type> v;
	function_call f_res = adapt_function_call(bm, m_logic_var, v, f);
	const function_def& def = funcs.get(f_res.id);

	adapt_res::permutationSeq perms;
	if (def.unify_predicate) {
		bool res = unify(bm, m_logic_var, f_res.args[0], f_res.args[1], perms);
		if (!res)
			return adapt_res::conflicting_facts();
		else {
			if (hyper::utils::all(perms.begin(), perms.end(), are_newly_introduced(v))) 
				return adapt_res::ok(f_res.args[0]);
			 else 
				return adapt_res::require_permutation(perms, f_res.args[0]);
		 }
	}

	return f_res;
}

const logic_var&
logic_var_db::get(const logic_var::identifier_type& id) const
{
	map_type::const_iterator it;
	it = m_logic_var.find(id);
	assert(it != m_logic_var.end());
	return it->second;
}

std::vector<function_call> 
logic_var_db::deadapt(const function_call& f) const
{
	std::vector<function_call> res;
	std::set<std::string> logic_vars;

	extract_logic_var(f, logic_vars);

	logic_var_to_exprM m;
	std::for_each(logic_vars.begin(), logic_vars.end(), generate_mapping(*this, m));

	logic_mappingM local_mapping;
	generate_funcall gen(logic_vars, m);
	bool has_next = true;
	while (has_next) {
		has_next = gen.next(local_mapping);
		res.push_back(replace(f, local_mapping));
	}

	return res;
}

} }
