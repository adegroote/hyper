#include <sstream>

#include <compiler/expression_ast.hh>

using namespace hyper::compiler;


struct node_ast_print : public boost::static_visitor<std::string>
{
	std::string operator() (const Constant<int>& i) const
	{
		std::ostringstream oss;
		oss << "Constant int of value : " << i.value;
		return oss.str();
	}

	std::string operator() (const Constant<double>& d) const
	{
		std::ostringstream oss;
		oss << "Constant double of value : " << d.value;
		return oss.str();
	};

	std::string operator() (const Constant<std::string>& s) const
	{
		std::ostringstream oss;
		oss << "Constant string of value : " << s.value;
		return oss.str();
	};

	std::string operator() (const Constant<bool>& b) const
	{
		std::ostringstream oss;
		oss << "Constant bool of value : " << b.value;
		return oss.str();
	}

	std::string operator() (const std::string& s) const
	{
		std::ostringstream oss;
		oss << "symbol named : " << s;
		return oss.str();
	}

	std::string operator() (const function_call& f) const
	{
		std::ostringstream oss;
		oss << "function call for function " << f.fName << " with args " << std::endl;
		for (size_t i = 0; i < f.args.size(); i++) {
			oss << boost::apply_visitor(node_ast_print(), f.args[i]);
			if ( i != f.args.size() - 1)
				oss << " ";
		}
		return oss.str();
	}
};

std::ostream& hyper::compiler::operator << (std::ostream& os, const node_ast& n)
{
	os << boost::apply_visitor(node_ast_print(), n) << std::endl;
	return os;
};

