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
