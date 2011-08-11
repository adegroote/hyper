#include <hyperConfig.hh>

#include <compiler/scope.hh>
#include <compiler/recipe_expression.hh>


using namespace hyper::compiler;

std::ostream& hyper::compiler::operator<< (std::ostream& os, const unification_expression& uni)
{
	os << uni.first << " == " << uni.second;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const logic_expression_decl& decl)
{
	os << decl.main;
	if (!decl.unification_clauses.empty()) {
		os << " where ";
		std::copy(decl.unification_clauses.begin(), decl.unification_clauses.end(),
				  std::ostream_iterator<unification_expression>(os, ", "));
	}

	return os;
}

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

struct extract_destination : public boost::static_visitor<boost::optional<std::string> >
{
	template <typename T>
	boost::optional<std::string> operator() (const T& t) const { return boost::none;}

	boost::optional<std::string> operator() (const std::string& sym) const
	{
		return scope::get_scope(sym);
	}

	boost::optional<std::string> operator() (const function_call& func) const
	{
		size_t i = 0;
		boost::optional<std::string> res = boost::none;
		while (!res && i < func.args.size())
			res = boost::apply_visitor(extract_destination(), func.args[i++].expr);

		return res;
	}

	template <unary_op_kind U>
	boost::optional<std::string> operator() (const unary_op<U>& unary) const 
	{
		return boost::apply_visitor(extract_destination(), unary.subject.expr);
	}

	template <binary_op_kind U>
	boost::optional<std::string> operator() (const binary_op<U>& binary) const 
	{
		boost::optional<std::string> res = boost::none;
		res = boost::apply_visitor(extract_destination(), binary.left.expr);
		if (!res)
			res = boost::apply_visitor(extract_destination(), binary.right.expr);
		return res;
	}
};

remote_constraint::remote_constraint(const expression_ast& ast) 
{
	logic_expr.main = ast;
	compute_dst();
}

remote_constraint::remote_constraint(const logic_expression_decl& decl) : logic_expr(decl)
{
	compute_dst();
}

void remote_constraint::compute_dst()
{
	dst = boost::apply_visitor(extract_destination(), logic_expr.main.expr);
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const remote_constraint& ctr)
{
	os << ctr.logic_expr;
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
