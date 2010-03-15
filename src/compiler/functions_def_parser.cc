
#include <compiler/functions_def_parser.hh>

using namespace hyper::compiler;

std::ostream& hyper::compiler::operator << (std::ostream& os, const function_decl& decl)
{
	os << "declaring function " << decl.fName << " returning type " << decl.returnName;
	os << "with args of type : " << std::endl;;

	std::vector < std::string >::const_iterator it;
	for (it = decl.argsName.begin(); it != decl.argsName.end(); ++it)
		os << "\t" << *it << std::endl;

	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const function_decl_list& l)
{
	std::vector < function_decl >::const_iterator it;
	for (it = l.l.begin(); it != l.l.end(); ++it)
		os << *it;

	return os;
}
