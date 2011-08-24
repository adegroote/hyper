#include <list>
#include <sstream>

#include <boost/bind.hpp>

#include <compiler/recipe_context.hh>
#include <compiler/recipe_expression.hh>

namespace {
using namespace hyper::compiler;

struct extract_local_vars_vis : public boost::static_visitor<void>
{
	std::vector<std::string>& local_vars;

	extract_local_vars_vis(std::vector<std::string>& local_vars) : 
		local_vars(local_vars) {}

	template <typename T>
	void operator() (const T& ) const {}

	void operator() (const let_decl& l) const {
		local_vars.push_back(l.identifier);
	}
};

struct extract_local_vars {
	std::vector<std::string>& local_vars;

	extract_local_vars(std::vector<std::string>& local_vars) : 
		local_vars(local_vars) {}

	void operator() (const recipe_expression& expr) {
		return boost::apply_visitor(extract_local_vars_vis(local_vars), expr.expr);
	}
};

struct make_recipe_fun_def {
	std::pair<std::string, recipe_fun_def> operator() (const fun_decl& decl) {
		return std::make_pair(decl.args[0], recipe_fun_def(decl));
	}
};

typedef std::map<std::string, std::string> symbol_mapping;

struct remap_symbol_vis : public boost::static_visitor<expression_ast>
{
	const symbol_mapping& map;

	remap_symbol_vis(const symbol_mapping& map) : map(map) {}

	template <typename T>
	expression_ast operator() (const T& t) const { return t; }

	expression_ast operator() (const expression_ast& e) const 
	{
		return boost::apply_visitor(*this, e.expr);
	}

	expression_ast operator() (const std::string& s) const
	{
		symbol_mapping::const_iterator it = map.find(s);
		if (it == map.end())
			return s;
		else
			return it->second;
	}

	expression_ast operator() (const function_call& f) const
	{
		function_call res(f);
		for (size_t i = 0; i < f.args.size(); ++i)
			res.args[i] = boost::apply_visitor(*this, f.args[i].expr);
		return res;
	}

	template <unary_op_kind K>
	expression_ast operator() (const unary_op<K>& op) const
	{
		unary_op<K> res(op);
		res.subject = boost::apply_visitor(*this, op.subject.expr);
		return res;
	}

	template <binary_op_kind K>
	expression_ast operator() (const binary_op<K>& op) const
	{
		binary_op<K> res(op);
		res.left = boost::apply_visitor(*this, op.left.expr);
		res.right = boost::apply_visitor(*this, op.right.expr);
		return res;
	}
};
						
expression_ast remap_symbol(const expression_ast& ast, const symbol_mapping& map)
{
	return boost::apply_visitor(remap_symbol_vis(map), ast.expr);
}

struct remap_unification {
	const symbol_mapping& map;

	remap_unification(const symbol_mapping& map) : map(map) {}

	void operator() (unification_expression& p) const {
		p.first = remap_symbol(p.first, map);
		p.second = remap_symbol(p.second, map);
	}
};

void remap_symbol_ctr(remote_constraint& ctr, const symbol_mapping& map)
{
	ctr.logic_expr.main = remap_symbol(ctr.logic_expr.main, map);
	std::for_each(ctr.logic_expr.unification_clauses.begin(), 
			      ctr.logic_expr.unification_clauses.end(),
				  remap_unification(map));
}

struct remap_symbol_recipe_vis : public boost::static_visitor<recipe_expression>
{
	const symbol_mapping& map;

	remap_symbol_recipe_vis(const symbol_mapping& map) : map(map) {}

	template <typename T>
	recipe_expression operator() (const T& t) const { return t; }

	recipe_expression operator() (const let_decl& l) const {
		let_decl res(l);
		symbol_mapping::const_iterator it = map.find(res.identifier);
		if (it != map.end())
			res.identifier = it->second;
		res.bounded = boost::apply_visitor(*this, l.bounded.expr);
		return res;
	};

	recipe_expression operator() (const abort_decl& a) const {
		symbol_mapping::const_iterator it = map.find(a.identifier);
		if (it == map.end())
			return a;
		else
			return abort_decl(it->second);
	};

	recipe_expression operator() (const set_decl& s) const {
		set_decl res(s);
		symbol_mapping::const_iterator it = map.find(res.identifier);
		if (it != map.end())
			res.identifier = it->second;
		res.bounded = remap_symbol(s.bounded, map);
		return res;
	}

	recipe_expression operator() (const wait_decl& w) const {
		return wait_decl(remap_symbol(w.content, map));
	}
	
	recipe_expression operator() (const expression_ast& ast) const {
		return remap_symbol(ast, map);
	}

	template <recipe_op_kind K>
	recipe_expression operator() (const recipe_op<K>& op) const {
		recipe_op<K> res(op);
		std::for_each(res.content.begin(), res.content.end(),
				      boost::bind(remap_symbol_ctr, _1, boost::cref(map)));
		return res;
	}
};

recipe_expression remap_symbol_recipe(const recipe_expression& exp, const symbol_mapping& map)
{
	symbol_mapping::const_iterator it;
	return boost::apply_visitor(remap_symbol_recipe_vis(map), exp.expr);
}

struct adapt_expression_to_context_helper : public boost::static_visitor<expression_ast>
{
	const recipe_context_decl::map_type& map;

	adapt_expression_to_context_helper(const recipe_context_decl::map_type& map) : map(map) {}

	template <typename T>
	expression_ast operator() (const T& t) const { return t; }

	expression_ast operator() (const std::string& s) const {
		recipe_context_decl::map_type::const_iterator it;
		it = map.find(s);
		if (it != map.end())
			return boost::apply_visitor(*this, it->second.expr);
		else
			return s;
	}

	expression_ast operator() (const expression_ast& e) const {
		return boost::apply_visitor(*this, e.expr);
	}

	expression_ast operator() (const function_call& f) const {
		function_call res(f);
		for (size_t i = 0; i < res.args.size(); ++i)
			res.args[i] = boost::apply_visitor(*this, res.args[i].expr);
		return res;
	}

	template <unary_op_kind K>
	expression_ast operator() (const unary_op<K>& op) const {
		unary_op<K> res(op);
		res.subject = boost::apply_visitor(*this, op.subject.expr);
		return res;
	}

	template <binary_op_kind K>
	expression_ast operator() (const binary_op<K>& op) const {
		binary_op<K> res(op);
		res.left = boost::apply_visitor(*this, op.left.expr);
		res.right = boost::apply_visitor(*this, op.right.expr);
		return res;
	}
};

struct adapt_recipe_condition_to_context_helper : public boost::static_visitor<recipe_condition> {
	const recipe_context_decl::map_type& map;

	adapt_recipe_condition_to_context_helper(const recipe_context_decl::map_type& map) :
		map(map)
	{}

	template <typename T>
	recipe_condition operator() (const T& t) const { return t; }

	recipe_condition operator() (const expression_ast& ast) {
		return adapt_expression_to_context(map)(ast);
	}
};


struct adapt_recipe_expression_to_context_helper : public boost::static_visitor<recipe_expression> {
	const recipe_context_decl::map_type& map;

	adapt_recipe_expression_to_context_helper(const recipe_context_decl::map_type& map) :
		map(map)
	{}

	template <typename T>
	recipe_expression operator() (const T& t) const { return t; }

	recipe_expression operator() (const let_decl& l) const {
		let_decl res(l);
		res.bounded = boost::apply_visitor(*this, l.bounded.expr);
		return res;
	}

	recipe_expression operator() (const set_decl& s) const {
		set_decl res(s);
		res.bounded = adapt_expression_to_context(map)(s.bounded.expr);
		return res;
	}

	recipe_expression operator() (const expression_ast& e) const {
		return adapt_expression_to_context(map)(e.expr);
	}

	recipe_expression operator() (const wait_decl& w) const {
		wait_decl res(w);
		res.content = adapt_expression_to_context(map)(res.content.expr);
		return res;
	}

	template <recipe_op_kind K>
	recipe_expression operator() (const recipe_op<K>& op) const {
		recipe_op<K> res(op);
		for (size_t i = 0; i < res.content.size(); ++i) {
			res.content[i].logic_expr.main =
				adapt_expression_to_context(map)(res.content[i].logic_expr.main.expr);
			res.content[i].compute_dst();
		}
		return res;
	}
};


}

namespace hyper {
	namespace compiler {
		recipe_fun_def::recipe_fun_def(const fun_decl& decl) {
			name = decl.args[0];
			std::copy(decl.args.begin() + 1, decl.args.end(), std::back_inserter(args));
			std::copy(decl.impl.list.begin(), decl.impl.list.end(),
					  std::back_inserter(impl));
			std::for_each(impl.begin(), impl.end(), extract_local_vars(local_vars));
		}

		map_fun_def make_map_fun_def(const recipe_context_decl& decl)
		{
			map_fun_def res;
			std::transform(decl.fun_defs.begin(), decl.fun_defs.end(),
						   std::inserter(res, res.end()),
						   make_recipe_fun_def());
			return res;
		}

		void apply_fun_body(std::vector<recipe_expression> & body_, const map_fun_def& map)
		{
			std::list<recipe_expression> body(body_.begin(), body_.end());
			std::list<recipe_expression>::iterator begin, end;
			begin = body.begin();
			end = body.end();
			size_t counter = 0;
			while (begin != end) {
				expression_ast* e = boost::get<expression_ast>(& begin->expr);
				if (e) {
					function_call* f = boost::get<function_call>(& e->expr);
					if (f) {
						map_fun_def::const_iterator it = map.find(f->fName);
						if (it != map.end()) 
						{
							assert(f->args.size() == it->second.args.size());
							symbol_mapping map;
							for (size_t i = 0; i < f->args.size(); ++i) {
								std::ostringstream arg_name;
								arg_name << "__fn" << counter << "_arg" << i; 
								body.insert(begin, let_decl(arg_name.str(), f->args[i]));
								map[it->second.args[i]] = arg_name.str();
							}
							for (size_t i = 0; i < it->second.local_vars.size(); ++i) {
								const std::string& s = it->second.local_vars[i];
								std::ostringstream remapped_name;
								remapped_name << "__fn" << counter << "_" << s;
								map[s] = remapped_name.str();
							}
							std::transform(it->second.impl.begin(), it->second.impl.end(),
										   std::inserter(body, begin),
										   boost::bind(remap_symbol_recipe, _1, boost::cref(map)));
							body.erase(begin);
							begin = body.begin();
							++counter;
							continue;
						}
					}
				}
				++begin;
			}

			body_.clear();
			std::copy(body.begin(), body.end(), std::back_inserter(body_));
		}

		expression_ast adapt_expression_to_context::operator() (const expression_ast& ast) const
		{
			return boost::apply_visitor(adapt_expression_to_context_helper(map), ast.expr);
		}

		recipe_expression adapt_recipe_expression_to_context::operator() (const recipe_expression& ast) const
		{
			return boost::apply_visitor(adapt_recipe_expression_to_context_helper(map), ast.expr);
		}

		recipe_condition adapt_recipe_condition_to_context::operator() (const recipe_condition& cond) const 
		{
			return boost::apply_visitor(adapt_recipe_condition_to_context_helper(map), cond.expr);
		}

	}
}
