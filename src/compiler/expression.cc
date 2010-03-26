#include <sstream>

#include <compiler/expression.hh>

using namespace hyper::compiler;


struct node_print : public boost::static_visitor<std::string>
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
			oss << boost::apply_visitor(node_print(), f.args[i]);
			if ( i != f.args.size() - 1)
				oss << " ";
		}
		return oss.str();
	}
};

std::ostream& hyper::compiler::operator << (std::ostream& os, const node& n)
{
	os << boost::apply_visitor(node_print(), n) << std::endl;
	return os;
};

