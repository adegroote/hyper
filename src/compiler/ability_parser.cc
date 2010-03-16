
#include <compiler/ability_parser.hh>

using namespace hyper::compiler;

std::ostream& hyper::compiler::operator << (std::ostream& os, const programming_decl& d)
{
	os << d.types;
	os << d.funcs;
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const ability_blocks_decl& d)
{
	os << " with controlable variables : " << std::endl << d.controlables;
	os << " with readable variables : " << std::endl << d.readables;
	os << " with private variables : " << std::endl << d.privates;
	os << d.env;
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const ability_decl& a)
{
	os << "ability " << a.name << "{" << std::endl;
	os << a.blocks;
	os << "}" << std::endl;
	return os;
}
