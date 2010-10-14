#include <compiler/ability.hh>
#include <compiler/expression_ast.hh>
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

template <unary_op_kind T>
struct ast_unary_valid 
{
	const universe& u;

	ast_unary_valid(const universe& u_) : u(u_) {};

	bool operator() (boost::optional<typeId> id) const
	{
		return false;
	}
};

template <>
struct ast_unary_valid<NEG>
{
	const universe& u;

	ast_unary_valid(const universe& u_) : u(u_) {};

	bool operator() (boost::optional<typeId> id) const
	{
		if (!id)
			return false;
		return (*id == u.types().getId("int").second || 
				*id == u.types().getId("double").second);
	}
};

enum valid_kind_of { VALID_NONE, VALID_NUMERICAL, VALID_COMPARABLE, VALID_LOGICAL };
template <binary_op_kind T> struct ValidTypeOp { enum { value = VALID_NONE }; };
template <> struct ValidTypeOp<ADD> { enum { value = VALID_NUMERICAL }; };
template <> struct ValidTypeOp<SUB> { enum { value = VALID_NUMERICAL }; };
template <> struct ValidTypeOp<MUL> { enum { value = VALID_NUMERICAL }; };
template <> struct ValidTypeOp<DIV> { enum { value = VALID_NUMERICAL }; };
template <> struct ValidTypeOp<AND> { enum { value = VALID_LOGICAL }; };
template <> struct ValidTypeOp<OR> { enum { value = VALID_LOGICAL }; };
template <> struct ValidTypeOp<GT> { enum { value = VALID_COMPARABLE }; };
template <> struct ValidTypeOp<GTE> { enum { value = VALID_COMPARABLE}; };
template <> struct ValidTypeOp<LT> { enum { value = VALID_COMPARABLE}; };
template <> struct ValidTypeOp<LTE> { enum { value = VALID_COMPARABLE}; };
template <> struct ValidTypeOp<EQ> { enum { value = VALID_COMPARABLE}; };
template <> struct ValidTypeOp<NEQ> { enum { value = VALID_COMPARABLE}; };

template <binary_op_kind T, int k>
struct ast_binary_valid_dispatch
{
	const universe& u;

	ast_binary_valid_dispatch(const universe& u_) : u(u_) {};

	bool operator() (boost::optional<typeId> leftType, boost::optional<typeId> rightType)
	{
		return false;
	}
};

template <binary_op_kind T>
struct ast_binary_valid_dispatch<T, VALID_NUMERICAL>
{
	const universe& u;

	ast_binary_valid_dispatch(const universe& u_) : u(u_) {};

	bool operator() (typeId leftType, typeId rightType)
	{
		// XXX Check that they are of "typeclass" numerical
		(void) leftType;
		(void) rightType;
		return true;
	}
};

template <binary_op_kind T>
struct ast_binary_valid_dispatch<T, VALID_COMPARABLE>
{
	const universe& u;

	ast_binary_valid_dispatch(const universe& u_) : u(u_) {};

	bool operator() (typeId leftType, typeId rightType)
	{
		// XXX check that they are of "typeclass" comparable
		(void) leftType;
		(void) rightType;
		return true;
	}
};

template <binary_op_kind T>
struct ast_binary_valid_dispatch<T, VALID_LOGICAL>
{
	const universe& u;

	ast_binary_valid_dispatch(const universe& u_) : u(u_) {};

	bool operator() (typeId leftType, typeId rightType)
	{
		(void) rightType;
		return (leftType == u.types().getId("bool").second);
	}
};

template <binary_op_kind T>
struct ast_binary_valid 
{
	const universe& u;
	
	ast_binary_valid(const universe& u_) : u(u_) {};

	bool operator() (boost::optional<typeId> leftType, boost::optional<typeId> rightType)
	{
		// only accept operation on same type
		if (!leftType || !rightType  || *leftType != *rightType) 
		{
			return false;
		}

		return ast_binary_valid_dispatch<T, ValidTypeOp<T>::value> (u) ( *leftType, *rightType );
	}
};

struct ast_valid : public boost::static_visitor<bool>
{
	const ability& ab;
	const universe& u;

	ast_valid(const ability& ab_, const universe& u_):
		ab(ab_), u(u_) 
	{}

	bool operator() (const empty& e) const
	{
		(void) e;
		return false;
	}

	template <typename T>
	bool operator() (const Constant<T>& c) const
	{
		(void) c;
		return true;
	}

	bool operator() (const std::string& s) const
	{
		std::pair<bool, symbolACL> p;
		p = u.get_symbol(s, ab);
		return p.first;
	}

	bool operator() (const function_call& f) const
	{
		// add scope to do the search
		std::string name = scope::add_scope(ab.name(), f.fName);
		std::pair<bool, functionDef> p = u.get_functionDef(name);
		if (p.first == false) {
			std::cerr << "Unknow function " << f.fName << std::endl;
			return false;
		}

		bool res = true;
		if (f.args.size() != p.second.arity()) {
			std::cerr << "Expected " << p.second.arity() << " arguments";
			std::cerr << " got " << f.args.size() << " for function ";
			std::cerr << f.fName << "!" << std::endl;
			res = false;
		}

		for (size_t i = 0; i < f.args.size(); ++i)
			res = boost::apply_visitor(ast_valid(ab, u), f.args[i].expr) && res;

		// check type
		for (size_t i = 0; i < f.args.size(); ++i) {
			boost::optional<typeId> id = u.typeOf(ab, f.args[i]);
			bool local_res = (id == p.second.argsType(i));
			res = local_res && res;
			if (local_res == false) {
				type expected = u.types().get(p.second.argsType(i));
				std::cerr << "Expected expression of type " << expected.name;
				if (!id) {
					std::cerr << " but can't compute type of " << f.args[i].expr;
				} else {
					type local = u.types().get(*id);
					std::cerr << " but get " << f.args[i].expr << " of type " << local.name;
				}
				std::cerr << " as argument " << i << " in the call of " << f.fName;
				std::cerr << std::endl;
			}
		}

		return res;
	}

	bool operator() (const expression_ast& e) const
	{
		return boost::apply_visitor(ast_valid(ab, u), e.expr);
	}

	template<binary_op_kind T>
	bool operator() (const binary_op<T>& b) const
	{
		bool left_valid, right_valid;
		left_valid = boost::apply_visitor(ast_valid(ab, u), b.left.expr);
		right_valid = boost::apply_visitor(ast_valid(ab, u), b.right.expr);
		return left_valid && right_valid && ast_binary_valid<T>(u) ( 
													u.typeOf(ab, b.left.expr),
													u.typeOf(ab, b.right.expr));
	}

	template<unary_op_kind T>
	bool operator() (const unary_op<T>& op) const
	{
		bool is_valid = boost::apply_visitor(ast_valid(ab, u), op.subject.expr);
		return is_valid && ast_unary_valid<T>(u) (u.typeOf(ab, op.subject.expr)) ;
	}
};

namespace hyper {
	namespace compiler {
		bool expression_ast::is_valid(const ability& context, const universe& u) const
		{
			ast_valid is_valid(context, u);
			return boost::apply_visitor(is_valid, this->expr);
		}
	}
}
