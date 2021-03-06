#include <compiler/condition_output.hh>
#include <compiler/expression_ast.hh>
#include <compiler/logic_expression_output.hh>
#include <compiler/output.hh>

using namespace hyper::compiler;

void generate_condition::operator() (const expression_ast& e) const
{
	oss << "\t\t\t\t(" << base << "_condition_" << counter++;
	oss << ", " << generate_logic_expression(e, a, u) << ")";
	oss << std::endl;
}


