#include <compiler/ability.hh>
#include <compiler/rules_def_parser.hh>
#include <compiler/universe.hh>

using namespace hyper::compiler;

std::ostream& hyper::compiler::operator << (std::ostream& os, const rule_decl& decl)
{
	os << decl.name << " : ";
	std::copy(decl.premises.begin(), decl.premises.end(), std::ostream_iterator<expression_ast>(os, ", "));
	os << " |- ";
	std::copy(decl.conclusions.begin(), decl.conclusions.end(), std::ostream_iterator<expression_ast>(os, ", "));
	os << ";\n";
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const rule_decl_list& l)
{
	std::copy(l.l.begin(), l.l.end(), std::ostream_iterator<rule_decl>(os));
	return os;
}
