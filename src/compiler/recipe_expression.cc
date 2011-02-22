#include <hyperConfig.hh>

#include <compiler/recipe_expression.hh>

using namespace hyper::compiler;

std::ostream& hyper::compiler::operator<< (std::ostream& os, const let_decl& e)
{
	os << "let " << e.identifier << " = " << e.bounded;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const abort_decl& a)
{
	os << "abort " << a.identifier;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const set_decl& s)
{
	os << "set " << s.identifier << " = " << s.bounded;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const wait_decl& w)
{
	os << "wait (" << w.content << ") ";
	return os;
}

struct recipe_expression_print : public boost::static_visitor<void>
{
	std::ostream& os;

	recipe_expression_print(std::ostream& os_) : os(os_) {}

	template <typename T>
	void operator() (const T& t) const
	{
		os << t;
	}

	void operator() (const empty&) const {}
};

std::ostream& hyper::compiler::operator<< (std::ostream& os, const recipe_expression& e)
{
	boost::apply_visitor(recipe_expression_print(os), e.expr);
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, recipe_op_kind kind)
{
	switch (kind) 
	{
		case MAKE:
			os << "make";
			break;
		case ENSURE:
			os << "ensure";
			break;
		default:
			assert(false);
	}

	return os;
}
