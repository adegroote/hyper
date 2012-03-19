#include <boost/bind.hpp>

#include <compiler/ability.hh>
#include <compiler/exec_expression_output.hh>
#include <compiler/output.hh>
#include <compiler/scope.hh>
#include <compiler/task.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>

using namespace hyper::compiler;

namespace {
	struct dump_expression_ast : public boost::static_visitor<std::string>
	{
		const std::set<std::string>& remote_syms;
		const symbolList& syms;
		std::string remote_access;
		std::string local_access;

		dump_expression_ast(const std::set<std::string>& remote_syms_,
				const symbolList& syms,
				const std::string& remote_access,
				const std::string& local_access):
			remote_syms(remote_syms_),
			syms(syms),
			remote_access(remote_access),
			local_access(local_access)
		{}

		std::string operator() (const empty& e) const { (void)e; assert(false); }

		std::string operator() (const Constant<int>& c) const
		{
			std::ostringstream oss;
			oss << c.value;
			return oss.str();
		}

		std::string operator() (const Constant<double>& c) const
		{
			std::ostringstream oss;
			oss.precision(6);
			oss << std::fixed << c.value;
			return oss.str();
		}

		std::string operator() (const Constant<std::string>& c) const
		{
			return quoted_string(c.value);
		}

		std::string operator() (const Constant<bool>& c) const
		{
			std::ostringstream oss;
			if (c.value)
				oss << "true";
			else 
				oss << "false";
			return oss.str();
		}

		std::string operator() (const std::string& s) const
		{
			std::ostringstream oss;
			std::set<std::string>::const_iterator it;
			it = remote_syms.find(s);
			// local scope variable
			if (it == remote_syms.end())
			{
				symbolList::const_iterator it = syms.find(s);
				if (it != syms.end()) {
					oss << local_access << "." << s ;
				} else {
					std::string real_name;
					if (!scope::is_scoped_identifier(s))
						real_name = s;
					else 
						real_name = (scope::decompose(s)).second;
					oss << "a." << real_name;
				}
			} else {
				// remote scope, the value will be available from entry N of the remote_proxy
				size_t i = std::distance(remote_syms.begin(), it);
				oss << "*( " << remote_access <<".at_c<" << i << ">())";
			}

			return oss.str();
		}

		std::string operator() (const function_call& f) const
		{
			std::ostringstream oss;
			oss << f.fName << "::apply(";
			for (size_t i = 0; i < f.args.size(); ++i) {
				oss << boost::apply_visitor(*this, f.args[i].expr);
				if (i != (f.args.size() - 1)) {
					oss << ",";
				}
			}
			oss << ")";
			return oss.str();
		}

		std::string operator() (const expression_ast& e) const
		{
			std::ostringstream oss;
			oss << "(";
			oss << boost::apply_visitor(*this, e.expr);
			oss << ")";
			return oss.str();
		}

		std::string operator() (const binary_op & op) const
		{
			std::ostringstream oss;
			oss << "( ";
			oss << "(";
			oss << boost::apply_visitor(*this, op.left.expr);
			oss << ")";
			oss << op.op;
			oss << "(";
			oss << boost::apply_visitor(*this, op.right.expr); 
			oss << ")";
			oss << " )";
			return oss.str();
		}

		std::string operator() (const unary_op& op) const
		{
			std::ostringstream oss;
			oss << "( ";
			oss << op.op;
			oss << boost::apply_visitor(*this, op.subject.expr);
			oss << ")";
			return oss.str();
		}
	};
}

void exec_expression_output::operator() (const expression_ast& e) 
{ 
	const std::string indent = "\t\t";
	const std::string indent_next = indent + "\t";

	std::string class_name = "hyper::" + ability_context.name() + "::ability";

	oss << indent << "void " << base_expr << "condition_" << counter++;
	oss << "(const " << class_name << " & a, " << std::endl;
	oss << indent_next << "bool& res, " << context_name << "::" ;
	oss << base_expr << "conditions& cond, size_t i)" << std::endl;
	oss << indent << "{" << std::endl;

	dump_expression_ast dump(remote_symbols, local_symbols, "cond.updater", "");
	std::string compute_expr = boost::apply_visitor(dump, e.expr);
	oss << indent_next << "res = " << compute_expr << ";" << std::endl;
	oss << indent_next << "return cond.handle_computation(i);" << std::endl;
	oss << indent << "}" << std::endl;
	oss << std::endl;
}

std::string hyper::compiler::expression_ast_output(const expression_ast& e,
												   const std::set<std::string>& remote,
												   const symbolList& syms)
{
	return boost::apply_visitor(dump_expression_ast(remote, syms, "updater", "local_vars"), e.expr);
}
