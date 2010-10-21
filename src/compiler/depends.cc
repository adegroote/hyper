#include <hyperConfig.hh>
#include <compiler/depends.hh>
#include <compiler/scope.hh>
#include <compiler/types.hh>

using namespace hyper::compiler;

struct compute_expression_deps : public boost::static_visitor<void>
{
	depends& d;	
	const std::string& name;

	compute_expression_deps(depends& d_, const std::string& name_) :
		d(d_), name(name_) {};

	template <typename T>
	void operator() (const T& e) const { (void)e; }

	void operator() (const expression_ast& e) const
	{
		boost::apply_visitor(compute_expression_deps(d, name), e.expr);
	}

	void operator() (const function_call& f) const
	{
		std::string scope = scope::get_scope(f.fName);
		if (scope == "") // a function without scope means that the function is part of the local scope
			scope = name;
		d.fun_depends.insert(scope);
		for (size_t i = 0; i < f.args.size(); ++i) 
			boost::apply_visitor(compute_expression_deps(d, name), f.args[i].expr);
	}

	template <binary_op_kind T>
	void  operator() (const binary_op<T> & op) const
	{
		boost::apply_visitor(compute_expression_deps(d, name), op.left.expr);
		boost::apply_visitor(compute_expression_deps(d, name), op.right.expr); 
	}

	template <unary_op_kind T>
	void operator() (const unary_op<T>& op) const
	{
		boost::apply_visitor(compute_expression_deps(d, name), op.subject.expr);
	}
};

namespace hyper {
	namespace compiler {
		void add_depends(const expression_ast& e, const std::string& context, depends& d)
		{
			boost::apply_visitor(compute_expression_deps(d, context), e.expr);
		}
	};
};
