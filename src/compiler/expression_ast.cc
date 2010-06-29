#include <iostream>
#include <sstream>

#include <compiler/expression_ast.hh>

using namespace hyper::compiler;

struct expression_ast_print : public boost::static_visitor<std::string>
{
	const int indent;

	expression_ast_print() : indent(0) {};
	expression_ast_print(int indent_) : indent(indent_) {};

	void add_indent(std::ostringstream& oss) const
	{
		for (int i = 0; i < indent; ++i)
			oss << "    ";
	}

	std::string operator() (const empty& e) const
	{
		(void) e;
		return "Empty";
	}

	template <typename T>
	std::string operator() (const Constant<T>& c) const
	{
		std::ostringstream oss;
		add_indent(oss);
		oss << c.value;
		return oss.str();
	}

	std::string operator() (const Constant<bool>& c) const
	{
		std::ostringstream oss;
		add_indent(oss);
		if (c.value)
			oss << "true";
		else 
			oss << "false";
		return oss.str();
	}

	std::string operator() (const std::string& s) const
	{
		std::ostringstream oss;
		add_indent(oss);
		oss << s;
		return oss.str();
	}

	std::string operator() (const function_call& f) const
	{
		std::ostringstream oss;
		add_indent(oss);
		oss << f.fName << "(" << std::endl;
		for (size_t i = 0; i < f.args.size(); ++i) {
			oss << boost::apply_visitor(expression_ast_print(indent+1), f.args[i].expr);
			if (i != (f.args.size() - 1)) {
				oss << "," << std::endl;
			}
		}
		oss << "\n";
		add_indent(oss);
		oss << ")";
		return oss.str();
	}

	std::string operator() (const expression_ast& e) const
	{
		return boost::apply_visitor(expression_ast_print(indent+1), e.expr);
	}

	template <binary_op_kind T>
	std::string operator() (const binary_op<T> & op) const
	{
		std::ostringstream oss;
		oss << boost::apply_visitor(expression_ast_print(indent+1), op.left.expr);
	    oss	<< std::endl;
		add_indent(oss);
		oss << op.op << std::endl;
		oss << boost::apply_visitor(expression_ast_print(indent+1), op.right.expr); 
		return oss.str();
	}

	template <unary_op_kind T>
	std::string operator() (const unary_op<T>& op) const
	{
		std::ostringstream oss;
		add_indent(oss);
		oss << op.op << std::endl;
		oss << boost::apply_visitor(expression_ast_print(indent+1), op.subject.expr);
		return oss.str();
	}
};

std::ostream& hyper::compiler::operator << (std::ostream& os, const expression_ast& e)
{
	os << boost::apply_visitor(expression_ast_print(0), e.expr);
	os << std::endl;
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const binary_op_kind& k)
{
	switch (k) {
		case ADD:
			os << " + ";
			break;
		case SUB:
			os << " - ";
			break;
		case MUL:
			os << " * ";
			break;
		case DIV:
			os << " / ";
			break;
		case EQ:
			os << " == ";
			break;
		case NEQ:
			os << " != ";
			break;
		case LT:
			os << " < " ;
			break;
		case LTE:
			os << " <= ";
			break;
		case GT:
			os << " > ";
			break;
		case GTE:
			os << " >= ";
			break;
		case AND:
			os << " && ";
			break;
		case OR:
			os << " || ";
			break;
		default:
			os << "You forget to implement me !!";
	}
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const unary_op_kind& k)
{
	switch (k) {
		case PLUS:
			os << " +";
			break;
		case NEG:
			os << " -";
			break;
		default:
			os << "You forget to implement me !!";
	}
	return os;
}

enum kind_of_op { NONE, NUMERICAL, LOGICAL, RELATIONAL, EQUALABLE };

template <typename T> struct is_numerical { enum {value = false}; };
template <> struct is_numerical<int> { enum { value = true }; };
template <> struct is_numerical<double> { enum { value = true }; };

template <typename T> struct is_bool { enum { value = false }; };
template <> struct is_bool<bool> { enum { value = true }; };

template <typename T> struct is_comparable { enum { value = false }; };
template <> struct is_comparable<int> { enum { value = true}; };
template <> struct is_comparable<double> { enum { value = true}; };
template <> struct is_comparable<std::string> { enum { value = true}; };

template <binary_op_kind T> struct TypeOp { enum { value = NONE }; };
template <> struct TypeOp<ADD> { enum { value = NUMERICAL }; };
template <> struct TypeOp<SUB> { enum { value = NUMERICAL }; };
template <> struct TypeOp<MUL> { enum { value = NUMERICAL }; };
template <> struct TypeOp<DIV> { enum { value = NUMERICAL }; };
template <> struct TypeOp<AND> { enum { value = LOGICAL }; };
template <> struct TypeOp<OR> { enum { value = LOGICAL }; };
template <> struct TypeOp<GT> { enum { value = RELATIONAL }; };
template <> struct TypeOp<GTE> { enum { value = RELATIONAL }; };
template <> struct TypeOp<LT> { enum { value = RELATIONAL }; };
template <> struct TypeOp<LTE> { enum { value = RELATIONAL }; };
template <> struct TypeOp<EQ> { enum { value = EQUALABLE }; };
template <> struct TypeOp<NEQ> { enum { value = EQUALABLE }; };


template <typename U, bool is_numerical> struct apply_negate {};

template <typename U> 
struct apply_negate<U, false> {
	expression_ast operator () (const Constant<U>& u) 
	{
		expression_ast res = unary_op<NEG>(u);
		std::cerr << "Invalid expression near " << res << std::endl;
		return res;
	}
};

template <typename U>
struct apply_negate<U, true> {
	expression_ast operator() (const Constant<U>& u)
	{
		return Constant<U>( - u.value );
	}
};


template <unary_op_kind T, typename U> struct apply_unary 
{
	expression_ast operator() (const Constant<U>& u)
	{
		std::cerr << "apply_unary is not implemented for " << T << std::endl;
		return empty();
	};
};

template <typename U> 
struct apply_unary<NEG, U> 
{
	expression_ast operator() (const Constant<U>& u)
	{
		return apply_negate<U, is_numerical<U>::value >() (u);
	}
};

template <unary_op_kind T>
struct ast_unary_reduce : public boost::static_visitor<expression_ast>
{
	template <typename U>
	expression_ast operator() (const U& u) const
	{
		return unary_op<T>(u);
	}

	template <typename U>
	expression_ast operator() (const Constant<U>& u) const
	{
		return apply_unary<T, U>()(u);
	}
};

/*
 * Binary visitation stuff
 * binary_apply<T, U> looks for the operation kind of T, and dispatch it to
 * binary_apply_dispatch<T, U, kind>
 *
 * binary_apply_dispatch<T, U, kind> checks that the type of U is compatible
 * with the kind of operation selected, and dispatch to the specialized version
 * binary_apply_equalable<T, U, bool>, binary_apply_numerical_op<T, U, bool>,
 * ...
 *
 * binary_apply_equalable<T,U, bool> prints some error message if the type
 * mismatch the requierement, otherwise, it calls binary_real_apply<T, U>
 *
 * binary_real_apply<T,U> does the real computation \o/
 *
 * Side note : pattern matching is cool, but it is a pain to write in C++
 * and this code take "hours" to compile (binary visitor is not a good idea)
 */
template <binary_op_kind T, typename U>
struct binary_apply_equalable {};

template <typename U>
struct binary_apply_equalable<EQ, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<bool>( u.value == v.value );
	}
};

template <typename U>
struct binary_apply_equalable<NEQ, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<bool>( u.value != v.value );
	}
};

template <binary_op_kind T, typename U, bool is_numerical>
struct binary_apply_numerical_op {};

template <binary_op_kind T, typename U>
struct binary_apply_numerical_op<T, U, false>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		expression_ast res = binary_op<T>(u,v);
		std::cerr << "Error near : " << res << " : expected numerical " << std::endl;
		return res;
	}
};

template <binary_op_kind T, typename U>
struct binary_real_apply
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		std::cerr << "binary_real_apply not implemented for " << T << std::endl;
		return empty();
	}
};

template < typename U>
struct binary_real_apply<ADD, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<U>( u.value + v.value );
	}
};

template < typename U>
struct binary_real_apply<SUB, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<U>( u.value - v.value );
	}
};

template < typename U>
struct binary_real_apply<MUL, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<U>( u.value * v.value );
	}
};

template < typename U>
struct binary_real_apply<DIV, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<U>( u.value / v.value );
	}
};

template <binary_op_kind T, typename U>
struct binary_apply_numerical_op<T, U, true>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return binary_real_apply<T, U>() (u,v);
	}	
};

template <binary_op_kind T, typename U, bool is_bool> struct binary_apply_logical {};
template <binary_op_kind T, typename U>
struct binary_apply_logical<T, U, false> 
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		expression_ast res = binary_op<T>(u, v);
		std::cerr << " Invalid expression near " << res << " expected bool ! " << std::endl;
		return res;
	}
};

template <typename U>
struct binary_real_apply<AND, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<bool> (u.value && v.value);
	}
};

template <typename U>
struct binary_real_apply<OR, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<bool> (u.value && v.value);
	}
};

template <binary_op_kind T, typename U>
struct binary_apply_logical<T, U, true>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return binary_real_apply<T, U>() (u, v);
	}
};

template <binary_op_kind T, typename U, bool is_comparable> struct binary_apply_relational {};
template <binary_op_kind T, typename U>
struct binary_apply_relational<T, U, false>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		expression_ast res = binary_op<T>(u, v);
		std::cerr << " Invalid expression near " << res << " expected comparable ! " << std::endl;
		return res;
	}
};

template <typename U>
struct binary_real_apply<LT, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<bool> (u.value < v.value);
	}
};

template <typename U>
struct binary_real_apply<LTE, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<bool> (u.value <= v.value);
	}
};

template <typename U>
struct binary_real_apply<GT, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<bool> (u.value > v.value);
	}
};

template <typename U>
struct binary_real_apply<GTE, U>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return Constant<bool> (u.value >= v.value);
	}
};

template <binary_op_kind T, typename U>
struct binary_apply_relational<T, U, true>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return binary_real_apply<T, U>() (u,v);
	}
};

template <binary_op_kind T, typename U, int kind>
struct binary_apply_dispatch
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		std::cerr << T << " has no kind identified " << std::endl;
		return empty();
	}
};

template <binary_op_kind T, typename U>
struct binary_apply_dispatch<T, U, EQUALABLE>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return binary_apply_equalable<T, U>() (u, v);
	}
};

template <binary_op_kind T , typename U>
struct binary_apply_dispatch<T, U, NUMERICAL>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return binary_apply_numerical_op<T, U, is_numerical<U>::value> () (u, v);
	}
};

template <binary_op_kind T, typename U>
struct binary_apply_dispatch<T, U, LOGICAL>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return binary_apply_logical<T, U, is_bool<U>::value> () (u, v);
	}
};

template <binary_op_kind T, typename U>
struct binary_apply_dispatch<T, U, RELATIONAL>
{
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v)
	{
		return binary_apply_relational< T, U, is_comparable<U>::value> () (u, v);
	}
};


template <binary_op_kind T, typename U>
struct binary_apply
{
	expression_ast operator () (const Constant<U>& u, const Constant<U>& v)
	{
		return binary_apply_dispatch< T, U, TypeOp<T>::value> () (u, v);
	}
};

template <binary_op_kind T>
struct ast_binary_reduce : public boost::static_visitor<expression_ast>
{
	template <typename U, typename V>
	expression_ast operator() ( const U& u, const V& v) const
	{
		return binary_op<T>(u, v);
	}

	template <typename U, typename V>
	expression_ast operator() (const Constant<U>& u, const Constant<V>& v) const
	{
		expression_ast res = binary_op<T>(u,v);
		std::cerr << "types mismatch near " << res << std::endl;
		return res;
	}

	template <typename U>
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v) const
	{
		return binary_apply<T, U>() (u,v);
	}
};

struct ast_reduce : public boost::static_visitor<expression_ast>
{
	expression_ast operator() (const empty& e) const
	{
		return e;
	}

	template <typename T>
	expression_ast operator() (const Constant<T>& c) const
	{
		return c;
	}

	expression_ast operator() (const expression_ast& e) const
	{
		return e.expr;
	}

	expression_ast operator() (const function_call& f) const
	{
		function_call f_res;
		f_res.fName = f.fName;
		for (std::size_t i = 0; i < f.args.size(); ++i)
			f_res.args.push_back(boost::apply_visitor(ast_reduce(), f.args[i].expr));
		return f_res;
	}

	template<binary_op_kind T>
	expression_ast operator() (const binary_op<T>& b) const
	{
		expression_ast left, right;
		left = boost::apply_visitor(ast_reduce(), b.left.expr);
		right = boost::apply_visitor(ast_reduce(), b.right.expr);
		return boost::apply_visitor(ast_binary_reduce<T>(), left.expr, right.expr);
	}

	template<unary_op_kind T>
	expression_ast operator() (const unary_op<T>& u) const
	{
		expression_ast subject = boost::apply_visitor(ast_reduce(), u.subject.expr);
		return boost::apply_visitor(ast_unary_reduce<T>(), subject.expr);
	}
};

void expression_ast::reduce()
{
	expr = boost::apply_visitor(ast_reduce(), expr);
}
