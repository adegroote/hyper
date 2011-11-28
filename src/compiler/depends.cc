#include <hyperConfig.hh>
#include <compiler/depends.hh>
#include <compiler/scope.hh>
#include <compiler/recipe_condition.hh>
#include <compiler/recipe_expression.hh>
#include <compiler/universe.hh>
#include <compiler/types.hh>

#include <boost/bind.hpp>

using namespace hyper::compiler;

namespace details {
	void add_depends(const expression_ast& e, const std::string& context, const universe& u, 
					 depends& d, const boost::optional<symbolList>& s);
}

struct compute_expression_deps : public boost::static_visitor<void>
{
	depends& d;	
	const std::string& name;
	const universe& u;
	const boost::optional<symbolList> &s;

	compute_expression_deps(depends& d_, const std::string& name_,
										 const universe& u,
										 const boost::optional<symbolList>& s) :
		d(d_), name(name_), u(u), s(s) {};

	template <typename T>
	void operator() (const T& e) const { (void)e; }

	void operator() (const expression_ast& e) const
	{
		boost::apply_visitor(*this, e.expr);
	}

	void operator() (const std::string& sym) const 
	{
		std::string scope = scope::get_scope(sym);
		if (scope == "")
			scope = name;
		d.var_depends.insert(scope);
		
		std::pair<bool, symbolACL> p;
		p = u.get_symbol(sym, u.get_ability(name), s);
		if (p.first == true) {
			typeId t = p.second.s.t;
			type t_ = u.types().get(t);
			d.type_depends.insert(scope::get_scope(t_.name));
		}
	}

	void operator() (const function_call& f) const
	{
		std::string scope = scope::get_scope(f.fName);
		std::string fullname = f.fName;
		if (scope == "") {// a function without scope means that the function is part of the local scope
			scope = name;
			fullname = scope::add_scope(name, fullname);
		}
		d.fun_depends.insert(scope);
		std::pair<bool, functionDef> p = u.funcs().get(fullname);
		assert(p.first);
		const boost::optional<std::string>& tag = p.second.tag();
		if (tag) 
			d.tag_depends.insert(*tag);
		for (size_t i = 0; i < f.args.size(); ++i) 
			boost::apply_visitor(*this, f.args[i].expr);
	}

	template <binary_op_kind T>
	void  operator() (const binary_op<T> & op) const
	{
		boost::apply_visitor(*this, op.left.expr);
		boost::apply_visitor(*this, op.right.expr); 
	}

	template <unary_op_kind T>
	void operator() (const unary_op<T>& op) const
	{
		boost::apply_visitor(*this, op.subject.expr);
	}
};

struct compute_recipe_op_deps 
{
	std::string name;
	const universe& u;
	depends &d;
	const symbolList& s;

	compute_recipe_op_deps(const std::string& name,
						   const universe& u, depends& d,
						   const symbolList& s):
		name(name), u(u), d(d), s(s)
	{}

	void operator() (const remote_constraint& constraint) const
	{
		details::add_depends(constraint.logic_expr.main, name, u, d, s);
	}
};

struct compute_recipe_condition_deps : public boost::static_visitor<void>
{
	depends& d;	
	const std::string& name;
	const universe& u;

	compute_recipe_condition_deps(depends& d_, const std::string& name_, 
											    const universe& u) :
		d(d_), name(name_), u(u) {}

	void operator() (const empty&) const {}

	void operator() (const expression_ast& ast) const {
		add_depends(ast, name, u, d);
	}

	void operator() (const last_error& e) const {
		add_depends(e.error, name, u, d);
	}
};

struct compute_recipe_expression_deps : public boost::static_visitor<void>
{
	depends& d;	
	const std::string& name;
	const universe& u;
	const symbolList &s;

	compute_recipe_expression_deps(depends& d_, const std::string& name_, 
											    const universe& u, 
												const symbolList& s) :
		d(d_), name(name_), u(u), s(s) {}

	template <typename T>
	void operator() (const T& e) const { (void)e; }

	void operator() (const let_decl& l) const 
	{
		boost::apply_visitor(*this, l.bounded.expr);
	}

	void operator() (const wait_decl& op) const 
	{
		details::add_depends(op.content, name, u, d, s);
	}

	template <recipe_op_kind kind>
	void operator() (const recipe_op<kind>& op) const
	{
		std::for_each(op.content.begin(), op.content.end(), 
					  compute_recipe_op_deps(name, u, d, s));
	}

	void operator() (const expression_ast& e) const
	{
		details::add_depends(e, name, u, d, s);
	}

	void operator() (const set_decl& s_) const
	{
		details::add_depends(s_.bounded, name, u, d, s);
	}
};

namespace details {
	void add_depends(const expression_ast& e, const std::string& context, const universe& u, 
					 depends& d, const boost::optional<symbolList>& s)
	{
		boost::apply_visitor(compute_expression_deps(d, context, u, s), e.expr);
	}
}

namespace hyper {
	namespace compiler {

		void add_depends(const expression_ast& e, const std::string& context, const universe& u, 
						 depends& d)
		{
			return details::add_depends(e, context, u, d, boost::none);
		}

		void add_depends(const recipe_condition& e, const std::string& context, const universe& u,
						 depends& d)
		{
			boost::apply_visitor(compute_recipe_condition_deps(d, context, u), e.expr);
		}

		void add_depends(const recipe_expression& e, const std::string& context, const universe& u, 
						 depends& d, const symbolList& s)
		{
			boost::apply_visitor(compute_recipe_expression_deps(d, context, u, s), e.expr);
		}
	}
}
