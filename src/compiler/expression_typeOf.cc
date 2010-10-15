#include <compiler/expression_ast.hh>
#include <compiler/recipe_expression.hh>
#include <compiler/scope.hh>
#include <compiler/types.hh>
#include <compiler/universe.hh>

using namespace hyper::compiler;

/*
 * Classify the binary_op into two kind
 *   - logical, returns a bool
 *   - numerical, the return type depends on the left operand (left and right
 *   operand must be the same)
 *   XXX : use "typeclass" to check that the operation has "sense"
 */

enum kind_of_op { NONE, NUMERICAL, LOGICAL};
template <binary_op_kind T> struct TypeOp { enum { value = NONE }; };
template <> struct TypeOp<ADD> { enum { value = NUMERICAL }; };
template <> struct TypeOp<SUB> { enum { value = NUMERICAL }; };
template <> struct TypeOp<MUL> { enum { value = NUMERICAL }; };
template <> struct TypeOp<DIV> { enum { value = NUMERICAL }; };
template <> struct TypeOp<AND> { enum { value = LOGICAL }; };
template <> struct TypeOp<OR> { enum { value = LOGICAL }; };
template <> struct TypeOp<GT> { enum { value = LOGICAL }; };
template <> struct TypeOp<GTE> { enum { value = LOGICAL }; };
template <> struct TypeOp<LT> { enum { value = LOGICAL }; };
template <> struct TypeOp<LTE> { enum { value = LOGICAL }; };
template <> struct TypeOp<EQ> { enum { value = LOGICAL }; };
template <> struct TypeOp<NEQ> { enum { value = LOGICAL }; };

template <binary_op_kind T, int k>
struct binary_type {
	const universe& u;
	boost::optional<typeId> id;	
	
	binary_type(const universe& u_, boost::optional<typeId> id_) : u(u_), id(id_) {};

	boost::optional<typeId> operator() () const { return boost::none; };
};

template <binary_op_kind T>
struct binary_type<T, NUMERICAL> {
	const universe& u;
	boost::optional<typeId> id;	
	
	binary_type(const universe& u_, boost::optional<typeId> id_) : u(u_), id(id_) {};

	boost::optional<typeId> operator() () const { return id; };
};

template <binary_op_kind T>
struct binary_type<T, LOGICAL> {
	const universe& u;
	boost::optional<typeId> id;	
	
	binary_type(const universe& u_, boost::optional<typeId> id_) : u(u_), id(id_) {};

	boost::optional<typeId> operator() () const { return u.types().getId("bool").second; };
};

/*
 * Compute the type of an expression
 * We assume that the expression is valid
 */
struct ast_type : public boost::static_visitor<boost::optional<typeId> > {
	const ability& ab;
	const universe& u;

	ast_type(const ability& ab_, const universe& u_):
		ab(ab_), u(u_) 
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
		p = u.get_symbol(s, ab);
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
		return boost::apply_visitor(ast_type(ab, u), e.expr);
	}

	template<binary_op_kind T>
	boost::optional<typeId> operator() (const binary_op<T>& b) const
	{
		boost::optional<typeId> leftId = boost::apply_visitor(ast_type(ab, u), b.left.expr);
		return binary_type<T, TypeOp<T>::value> (u, leftId) ();
	}

	boost::optional<typeId> operator() (const unary_op<NEG>& op) const
	{
		(void) op;
		return u.types().getId("bool").second;
	}
};

boost::optional<typeId>
universe::typeOf(const ability& ab, const expression_ast& expr) const
{
	return boost::apply_visitor(ast_type(ab, *this), expr.expr);
}

/* 
 * Compute the type of a recipe_expression
 * Assume that the recipe_expression is valid
 */
struct recipe_ast_type : public boost::static_visitor<boost::optional<typeId> > {
	const ability& ab;
	const universe& u;

	recipe_ast_type(const ability& ab_, const universe& u_):
		ab(ab_), u(u_) 
	{}

	boost::optional<typeId>
	operator() (const empty&) const { assert(false); return boost::none; }

	boost::optional<typeId>
	operator() (const expression_ast& e) const {
		return u.typeOf(ab, e);
	}

	boost::optional<typeId> 
	operator() (const let_decl&) const {
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
	operator() (const recipe_op<WAIT>&) const {
		return u.types().getId("void").second;
	}
};

boost::optional<typeId>
universe::typeOf(const ability& ab, const recipe_expression& expr) const
{
	return boost::apply_visitor(recipe_ast_type(ab, *this), expr.expr);
}
