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

	std::string operator() (const binary_op & op) const
	{
		std::ostringstream oss;
		oss << boost::apply_visitor(expression_ast_print(indent+1), op.left.expr);
	    oss	<< std::endl;
		add_indent(oss);
		oss << op.op << std::endl;
		oss << boost::apply_visitor(expression_ast_print(indent+1), op.right.expr); 
		return oss.str();
	}

	std::string operator() (const unary_op& op) const
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

template <typename T> struct is_numerical { enum {value = false}; };
template <> struct is_numerical<int> { enum { value = true }; };
template <> struct is_numerical<double> { enum { value = true }; };

template <typename T> struct is_bool { enum { value = false }; };
template <> struct is_bool<bool> { enum { value = true }; };

template <typename T> struct is_comparable { enum { value = false }; };
template <> struct is_comparable<int> { enum { value = true}; };
template <> struct is_comparable<double> { enum { value = true}; };
template <> struct is_comparable<std::string> { enum { value = true}; };

expression_ast unary_apply(unary_op_kind k, const Constant<bool>& u)
{
	assert(false);
	return empty();
}

expression_ast unary_apply(unary_op_kind k, const Constant<std::string>& u)
{
	assert(false);
	return empty();
}

template <typename U>
expression_ast unary_apply(unary_op_kind k, const Constant<U>& u)
{
	switch(k) {
		case PLUS:
			return Constant<U> (u.value);
		case NEG:
			return Constant<U> (-u.value);
	}
}

struct ast_unary_reduce : public boost::static_visitor<expression_ast>
{
	unary_op_kind k;


	ast_unary_reduce(unary_op_kind k_) : k(k_) {}

	template <typename U>
	expression_ast operator() (const U& u) const
	{
		return unary_op(k, u);
	}

	template <typename U>
	expression_ast operator() (const Constant<U>& u) const
	{
		unary_op res(k, u);
		switch (k) {
			case NEG:
				if ( is_numerical<U>::value )  {
					std::cerr << "Invalid expression near " << res << std::endl;
					return unary_apply(k, u);
				} else 
					return res;
			default:
				assert(false);
				return res;
		}
	}
};

expression_ast real_apply(binary_op_kind k, const Constant<bool>& u, const Constant<bool>& v)
{
	switch(k) {
		case AND:
			return Constant<bool> (u.value && v.value);
		case OR:
			return Constant<bool> (u.value || v.value);
		case EQ:
			return Constant<bool> (u.value == v.value);
		case NEQ:
			return Constant<bool> (u.value != v.value);
		default:
			assert(false);
			return empty();
	}
}

expression_ast real_apply(binary_op_kind k, const Constant<std::string>& u, const Constant<std::string> &v)
{
	switch(k) {
		case EQ:
			return Constant<bool> (u.value == v.value);
		case NEQ:
			return Constant<bool> (u.value != v.value);
		case LT:
			return Constant<bool> (u.value < v.value);
		case LTE:
			return Constant<bool> (u.value <= v.value);
		case GT:
			return Constant<bool> (u.value > v.value);
		case GTE:
			return Constant<bool> (u.value >= v.value);
		default:
			assert(false);
			return empty();
	}
}


template <typename U>
expression_ast real_apply(binary_op_kind k, const Constant<U>& u, const Constant<U>& v)
{
	switch(k) {
		case ADD:
			return Constant<U> (u.value + v.value);
		case SUB:
			return Constant<U> (u.value - v.value);
		case MUL:
			return Constant<U> (u.value * v.value);
		case DIV:
			return Constant<U> (u.value / v.value);
		case EQ:
			return Constant<bool> (u.value == v.value);
		case NEQ:
			return Constant<bool> (u.value != v.value);
		case LT:
			return Constant<bool> (u.value < v.value);
		case LTE:
			return Constant<bool> (u.value <= v.value);
		case GT:
			return Constant<bool> (u.value > v.value);
		case GTE:
			return Constant<bool> (u.value >= v.value);
		default:
			assert(false);
			return empty();
	}
}

struct ast_binary_reduce : public boost::static_visitor<expression_ast>
{
	binary_op_kind k;

	ast_binary_reduce(binary_op_kind k_) : k(k_) {}

	template <typename U, typename V>
	expression_ast operator() ( const U& u, const V& v) const
	{
		return binary_op(k, u, v);
	}

	template <typename U, typename V>
	expression_ast operator() (const Constant<U>& u, const Constant<V>& v) const
	{
		expression_ast res = binary_op(k, u,v);
		std::cerr << "types mismatch near " << res << std::endl;
		return res;
	}

	template <typename U>
	expression_ast operator() (const Constant<U>& u, const Constant<U>& v) const
	{
		expression_ast res = binary_op(k, u, v);

		switch (k) {
			case ADD:
			case SUB:
			case MUL:
			case DIV:
				if (! is_numerical<U>::value ) {
					std::cerr << "Error near : " << res << " : expected numerical " << std::endl;
					return res;
				} else
					return real_apply(k, u, v);

			case AND:
			case OR:
				if (! is_bool<U>::value ) {
					std::cerr << " Invalid expression near " << res << " expected bool ! " << std::endl;
					return res;
				} else
					return real_apply(k, u, v);

			case GT:
			case GTE:
			case LT:
			case LTE:
				if (! is_comparable<U>::value ) {
					std::cerr << " Invalid expression near " << res << " expected comparable ! " << std::endl;
					return res;
				}
					return real_apply(k, u, v);

			case EQ:
			case NEQ:
					return real_apply(k, u, v);
			default:
					assert(false);
					return res;
		}
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

	expression_ast operator() (const binary_op& b) const
	{
		expression_ast left, right;
		left = boost::apply_visitor(ast_reduce(), b.left.expr);
		right = boost::apply_visitor(ast_reduce(), b.right.expr);
		return boost::apply_visitor(ast_binary_reduce(b.op), left.expr, right.expr);
	}

	expression_ast operator() (const unary_op& u) const
	{
		expression_ast subject = boost::apply_visitor(ast_reduce(), u.subject.expr);
		return boost::apply_visitor(ast_unary_reduce(u.op), subject.expr);
	}
};

void expression_ast::reduce()
{
	expr = boost::apply_visitor(ast_reduce(), expr);
}
