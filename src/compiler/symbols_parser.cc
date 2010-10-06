#include <hyperConfig.hh>
#include <compiler/symbols_parser.hh>

using namespace hyper::compiler;

std::ostream& hyper::compiler::operator << (std::ostream& os, const symbol_decl& s)
{
	os << " declaration of symbol " << s.name << " of type " << s.typeName << std::endl;
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const symbol_decl_list& l)
{
	std::vector<symbol_decl>::const_iterator it;
	for (it = l.l.begin(); it != l.l.end(); ++it) 
		os << *it;
	return os;
}

