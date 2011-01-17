#include <compiler/expression_ast.hh>
#include <compiler/extract_symbols.hh>
#include <compiler/scope.hh>

namespace {
	using namespace hyper::compiler;
	/* Extract the list of a symbol from a compiler::expression_ast */
	struct extract_syms_helper : public boost::static_visitor<void>
	{

		extract_symbols& syms;

		extract_syms_helper(extract_symbols& syms) :
			syms(syms)
		{}

		template <typename T>
		void operator() (const T&) const{}

		void operator() (const std::string& sym) const
		{
			if (scope::is_scoped_identifier(sym)) {
				std::pair<std::string, std::string> p;
				p = scope::decompose(sym);
				if (p.first == syms.context) 
					syms.local.insert(p.second);
				else
					syms.remote.insert(sym);
			} else {
				syms.local.insert(sym);
			}
		}

		void operator() (const expression_ast& e) const
		{
			boost::apply_visitor(extract_syms_helper(syms), e.expr);
		}

		void operator() (const function_call& f) const
		{
			for (size_t i = 0; i < f.args.size(); ++i) 
				boost::apply_visitor(extract_syms_helper(syms), f.args[i].expr);
		}
	
		template <binary_op_kind T>
		void  operator() (const binary_op<T> & op) const
		{
			boost::apply_visitor(extract_syms_helper(syms), op.left.expr);
			boost::apply_visitor(extract_syms_helper(syms), op.right.expr); 
		}
	
		template <unary_op_kind T>
		void operator() (const unary_op<T>& op) const
		{
			boost::apply_visitor(extract_syms_helper(syms), op.subject.expr);
		}
	};
}

namespace hyper {
	namespace compiler {
		extract_symbols::extract_symbols(const std::string& context) : context(context) {}

		void extract_symbols::extract(const expression_ast& e)
		{
			boost::apply_visitor(extract_syms_helper(*this), e.expr);
		}
	}
}
