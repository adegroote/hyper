#include <hyperConfig.hh>
#include <compiler/depends.hh>
#include <compiler/scope.hh>
#include <compiler/recipe_expression.hh>
#include <compiler/universe.hh>
#include <compiler/types.hh>

#include <boost/bind.hpp>

using namespace hyper::compiler;

struct compute_expression_deps : public boost::static_visitor<void>
{
	depends& d;	
	const std::string& name;
	const universe& u;

	compute_expression_deps(depends& d_, const std::string& name_,
										 const universe& u) :
		d(d_), name(name_), u(u) {};

	template <typename T>
	void operator() (const T& e) const { (void)e; }

	void operator() (const expression_ast& e) const
	{
		boost::apply_visitor(compute_expression_deps(d, name, u), e.expr);
	}

	void operator() (const std::string& s) const 
	{
		std::string scope = scope::get_scope(s);
		if (scope == "")
			scope = name;
		d.var_depends.insert(scope);
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
			boost::apply_visitor(compute_expression_deps(d, name, u), f.args[i].expr);
	}

	template <binary_op_kind T>
	void  operator() (const binary_op<T> & op) const
	{
		boost::apply_visitor(compute_expression_deps(d, name, u), op.left.expr);
		boost::apply_visitor(compute_expression_deps(d, name, u), op.right.expr); 
	}

	template <unary_op_kind T>
	void operator() (const unary_op<T>& op) const
	{
		boost::apply_visitor(compute_expression_deps(d, name, u), op.subject.expr);
	}
};

struct compute_recipe_expression_deps : public boost::static_visitor<void>
{
	depends& d;	
	const std::string& name;
	const universe& u;

	compute_recipe_expression_deps(depends& d_, const std::string& name_, 
											    const universe& u) :
		d(d_), name(name_), u(u) {}

	template <typename T>
	void operator() (const T& e) const { (void)e; }

	void operator() (const let_decl& l) const 
	{
		boost::apply_visitor(compute_recipe_expression_deps(d, name, u), l.bounded.expr);
	}

	void operator() (const wait_decl& op) const 
	{
		add_depends(op.content, name, u, d);
	}

	template <recipe_op_kind kind>
	void operator() (const recipe_op<kind>& op) const
	{
		void (*f)(const expression_ast&, const std::string&, const universe& u, depends&) = 
			&add_depends;

		std::for_each(op.content.begin(), op.content.end(), 
				boost::bind(f, _1, boost::cref(name), 
								   boost::cref(u),
											  boost::ref(d)));

	}

	void operator() (const expression_ast& e) const
	{
		add_depends(e, name, u, d);
	}

	void operator() (const set_decl& s) const
	{
		add_depends(s.bounded, name, u, d);
	}
};

namespace hyper {
	namespace compiler {
		void add_depends(const expression_ast& e, const std::string& context, const universe& u, 
						 depends& d)
		{
			boost::apply_visitor(compute_expression_deps(d, context, u), e.expr);
		}

		void add_depends(const recipe_expression& e, const std::string& context, const universe& u, 
						 depends& d)
		{
			boost::apply_visitor(compute_recipe_expression_deps(d, context, u), e.expr);
		}
	}
}
