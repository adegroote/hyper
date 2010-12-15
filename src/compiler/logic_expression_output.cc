#include <compiler/logic_expression_output.hh>
#include <compiler/expression_ast.hh>
#include <compiler/output.hh>

using namespace hyper::compiler;

namespace {
	template <binary_op_kind op> struct logic_function {};
	template <> struct logic_function<ADD> { static std::string name() { return "add";}};
	template <> struct logic_function<SUB> { static std::string name() { return "minus";}};
	template <> struct logic_function<MUL> { static std::string name() { return "times";}};
	template <> struct logic_function<DIV> { static std::string name() { return "divides";}};
	template <> struct logic_function<EQ> { static std::string name() { return "equal";}};
	template <> struct logic_function<NEQ>{ static std::string name() { return "not_equal";}};
	template <> struct logic_function<LT> { static std::string name() { return"less";}};
	template <> struct logic_function<LTE>{ static std::string name() { return "less_equal";}};
	template <> struct logic_function<GT> { static std::string name() { return"greater";}};
	template <> struct logic_function<GTE>{ static std::string name() { return "geater_equal";}};
	template <> struct logic_function<AND>{ static std::string name() { return "and";}};
	template <> struct logic_function<OR> { static std::string name() { return "or";}};

	template <unary_op_kind op> struct logic_unary_function {};
	template <> struct logic_unary_function<NEG> { static std::string name() { return "negate";}};
	
	struct dump_logic_expression_ast  : public boost::static_visitor<std::string>
	{
		std::string operator() (const empty& e) const { (void)e; assert(false); }

		template <typename T>
		std::string operator() (const Constant<T>& c) const
		{
			std::ostringstream oss;
			oss << c.value;
			return oss.str();
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
			return s;
		}

		std::string operator() (const function_call& f) const
		{
			std::ostringstream oss;
			(void) f;
			oss << f.fName << "(";
			for (size_t i = 0; i < f.args.size(); ++i) {
				oss << boost::apply_visitor(dump_logic_expression_ast(), f.args[i].expr);
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
			oss << boost::apply_visitor(dump_logic_expression_ast(), e.expr);
			oss << ")";
			return oss.str();
		}

		template <binary_op_kind T>
		std::string operator() (const binary_op<T> & op) const
		{
			std::ostringstream oss;
			oss << logic_function<T>::name();
			oss << "(";
			oss << boost::apply_visitor(dump_logic_expression_ast(), op.left.expr);
			oss << ",";
			oss << boost::apply_visitor(dump_logic_expression_ast(), op.right.expr); 
			oss << ")";
			return oss.str();
		}

		template <unary_op_kind T>
		std::string operator() (const unary_op<T>& op) const
		{
			std::ostringstream oss;
			oss << "(";
			oss << boost::apply_visitor(dump_logic_expression_ast(), op.subject.expr);
			oss << ")";
			return oss.str();
		}
	};
}
namespace hyper {
	namespace compiler {
		std::string generate_logic_expression(const expression_ast& e)
		{
			return boost::apply_visitor(dump_logic_expression_ast(), e.expr);
		}
	}
}
