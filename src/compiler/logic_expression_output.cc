#include <compiler/logic_expression_output.hh>
#include <compiler/expression_ast.hh>
#include <compiler/output.hh>
#include <compiler/scope.hh>
#include <compiler/universe.hh>

using namespace hyper::compiler;

namespace {

	template <unary_op_kind op> struct logic_unary_function {};
	template <> struct logic_unary_function<NEG> { static std::string name() { return "negate";}};
	
	struct dump_logic_expression_ast  : public boost::static_visitor<std::string>
	{
		const ability& a;
		boost::optional<const universe&> u;
		bool is_rule;

		dump_logic_expression_ast(const ability& a, boost::optional<const universe&> u, bool is_rule) : 
			a(a), u(u), is_rule(is_rule)
		{}

		std::string operator() (const empty& e) const { (void)e; assert(false); }

		std::string operator() (const Constant<int>& c) const
		{
			std::ostringstream oss;
			oss << "logic::Constant<int>(" << c.value << ")";
			return oss.str();
		}

		std::string operator() (const Constant<double>& c) const
		{
			std::ostringstream oss;
			oss.precision(6);
			oss << "logic::Constant<double>(" << std::fixed << c.value << ")";
			return oss.str();
		}

		std::string operator() (const Constant<std::string>& c) const
		{
			std::ostringstream oss;
			oss << "logic::Constant<std::string>(\"" << c.value << "\")";
			return oss.str();
		}

		std::string operator() (const Constant<bool>& c) const
		{
			std::ostringstream oss;
			oss << "logic::Constant<bool>(";
			if (c.value)
				oss << "true";
			else 
				oss << "false";
			oss << ")";
			return oss.str();
		}

		std::string operator() (const std::string& s) const
		{
			std::ostringstream oss;
			oss << "std::string(\"";
			if (!is_rule && !scope::is_scoped_identifier(s))
				oss << a.name() <<  "::" << s;
			else
				oss << s;
			oss << "\")";
			return oss.str();
		}

		std::string operator() (const function_call& f) const
		{
			std::ostringstream oss;
			oss << "logic::function_call(\"";
			if (scope::is_basic_identifier(f.fName) || scope::is_scoped_identifier(f.fName))
				oss << f.fName << "\", ";
			else 
				oss << a.name() << "::" << f.fName << "\", ";
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
			return boost::apply_visitor(*this, e.expr);
		}

		std::string operator() (const binary_op & op) const
		{
			std::ostringstream oss;
			oss << "logic::function_call(\"";
			oss << logic_function_name(op.op);
			if (u) {
				boost::optional<typeId> tid = (*u).typeOf(a, op.left.expr);
				if (tid) {
					type t = (*u).types().get(*tid);
					oss << "_" << t.name;
				}
			}
			oss << "\", ";
			oss << boost::apply_visitor(*this, op.left.expr);
			oss << ",";
			oss << boost::apply_visitor(*this, op.right.expr); 
			oss << ")";
			return oss.str();
		}

		std::string operator() (const unary_op& op) const
		{
			std::ostringstream oss;
			oss << "(";
			oss << boost::apply_visitor(*this, op.subject.expr);
			oss << ")";
			return oss.str();
		}
	};
}

namespace hyper {
	namespace compiler {
		std::string logic_function_name(binary_op_kind k)
		{
			switch (k) {
				case ADD:
					return "add";
				case SUB:
					return "minus";
				case MUL:
					return "times";
				case DIV:
					return "divides";
				case EQ:
					return "equal";
				case NEQ:
					return "not_equal";
				case LT:
					return "less";
				case LTE:
					return "less_equal";
				case GT:
					return "greater";
				case GTE:
					return "greater_equal";
				case AND:
					return "and";
				case OR:
					return "or";
				default:
					assert(false);
			}
		}

		std::string generate_logic_expression(const expression_ast& e, const ability& a, 
				boost::optional<const universe&> u, bool is_rule)
		{
			return boost::apply_visitor(dump_logic_expression_ast(a, u, is_rule), e.expr);
		}
	}
}
