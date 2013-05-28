#include <compiler/expression_ast.hh>
#include <compiler/recipe_expression.hh>
#include <compiler/scope.hh>
#include <compiler/types.hh>
#include <compiler/universe.hh>

using namespace hyper::compiler;

/*
 * Compute the type of an expression
 * We assume that the expression is valid
 */
struct ast_type : public boost::static_visitor<boost::optional<typeId> > {
	const ability& ab;
	const universe& u;
	const boost::optional<symbolList>& local_context;

	ast_type(const ability& ab_, const universe& u_, 
			const boost::optional<symbolList>& local_context_):
		ab(ab_), u(u_), local_context(local_context_)
	{}

	boost::optional<typeId> operator() (const empty& e) const
	{
		(void) e;
		return boost::none;
	}

	boost::optional<typeId> operator() (const Constant<int>& c) const
	{
		(void) c;
		return u.types().getId("int").second;
	}

	boost::optional<typeId> operator() (const Constant<double>& c) const
	{
		(void) c;
		return u.types().getId("double").second;
	}

	boost::optional<typeId> operator() (const Constant<std::string>& c) const
	{
		(void) c;
		return u.types().getId("string").second;
	}

	boost::optional<typeId> operator() (const Constant<bool>& c) const
	{
		(void) c;
		return u.types().getId("bool").second;
	}

	boost::optional<typeId> operator() (const std::string& s) const
	{
		std::pair<bool, symbolACL> p;
		p = u.get_symbol(s, ab, local_context);
		if (p.first == false)
			return boost::none;
		return p.second.s.t; 
	}

	boost::optional<typeId> operator() (const function_call& f) const
	{
		// add scope to do the search
		std::string name = scope::add_scope(ab.name(), f.fName);
		std::pair<bool, functionDef> p = u.get_functionDef(name);
		if (p.first == false) 
			return boost::none;

		return p.second.returnType();
	}

	boost::optional<typeId> operator() (const expression_ast& e) const
	{
		return boost::apply_visitor(ast_type(ab, u, local_context), e.expr);
	}

	boost::optional<typeId> operator() (const binary_op& b) const
	{
		boost::optional<typeId> leftId = boost::apply_visitor(ast_type(ab, u, local_context), b.left.expr);
		switch(b.op) {
			case ADD:
			case SUB:
			case MUL:
			case DIV:
				return leftId;
			case AND:
			case OR:
			case GT:
			case GTE:
			case LT:
			case LTE:
			case EQ:
			case NEQ:
				return u.types().getId("bool").second; 
			default:
				return boost::none;
		}
	}

	boost::optional<typeId> operator() (const unary_op& op) const
	{
		return boost::apply_visitor(ast_type(ab, u, local_context), op.subject.expr);
	}
};

boost::optional<typeId>
universe::typeOf(const ability& ab, const expression_ast& expr,
				 const boost::optional<symbolList>& local_context) const
{
	return boost::apply_visitor(ast_type(ab, *this, local_context), expr.expr);
}

/* 
 * Compute the type of a recipe_expression
 * Assume that the recipe_expression is valid
 */
struct recipe_ast_type : public boost::static_visitor<boost::optional<typeId> > {
	const ability& ab;
	const universe& u;
	const boost::optional<symbolList>& local_context;

	recipe_ast_type(const ability& ab_, const universe& u_,
					const boost::optional<symbolList>& local_context_) :
		ab(ab_), u(u_), local_context(local_context_)
	{}

	boost::optional<typeId>
	operator() (const empty&) const { assert(false); return boost::none; }

	boost::optional<typeId>
	operator() (const expression_ast& e) const {
		return u.typeOf(ab, e, local_context);
	}

	boost::optional<typeId> 
	operator() (const let_decl&) const {
		return u.types().getId("void").second;
	}

	boost::optional<typeId> 
	operator() (const set_decl&) const {
		return u.types().getId("void").second;
	}

	boost::optional<typeId>
	operator() (const abort_decl&) const {
		return u.types().getId("void").second;
	}

	boost::optional<typeId>
	operator() (const recipe_op<MAKE>&) const {
		return u.types().getId("bool").second;
	}

	boost::optional<typeId>
	operator() (const recipe_op<ENSURE>&) const {
		return u.types().getId("identifier").second;
	}

	boost::optional<typeId>
	operator() (const observer_op<WAIT>&) const {
		return u.types().getId("void").second;
	}

	boost::optional<typeId>
	operator() (const observer_op<ASSERT>&) const {
		return u.types().getId("identifier").second;
	}

	boost::optional<typeId>
	operator() (const while_decl&) const {
		return u.types().getId("void").second;
	}
};

boost::optional<typeId>
universe::typeOf(const ability& ab, const recipe_expression& expr,
				 const boost::optional<symbolList>& local_context) const
{
	return boost::apply_visitor(recipe_ast_type(ab, *this, local_context), expr.expr);
}
