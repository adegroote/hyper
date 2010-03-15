
#include <compiler/ability_parser.hh>

using namespace hyper::compiler;

std::ostream& hyper::compiler::operator << (std::ostream& os, const programming_decl& d)
{
	os << d.types;
	os << d.funcs;
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const ability_decl& a)
{
	os << "ability " << a.name << "{" << std::endl;
	os << " with controlable variables : " << std::endl << a.controlables;
	os << " with readable variables : " << std::endl << a.readables;
	os << " with private variables : " << std::endl << a.privates;
	os << a.env;
	os << "}" << std::endl;
	return os;
}
