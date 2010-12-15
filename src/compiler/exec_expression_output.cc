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
	/* Extract the list of a symbol from a compiler::expression_ast */
	struct symbol_list : public boost::static_visitor<void>
	{
		std::vector<std::string> &sym_list;

		symbol_list(std::vector<std::string>& list) :
			sym_list(list)
		{}

		template <typename T>
		void operator() (const T& x) const
		{
			(void) x;
		}

		void operator() (const std::string& sym) const
		{
			sym_list.push_back(sym);
		}

		void operator() (const expression_ast& e) const
		{
			boost::apply_visitor(symbol_list(sym_list), e.expr);
		}

		void operator() (const function_call& f) const
		{
			for (size_t i = 0; i < f.args.size(); ++i) 
				boost::apply_visitor(symbol_list(sym_list), f.args[i].expr);
		}
	
		template <binary_op_kind T>
		void  operator() (const binary_op<T> & op) const
		{
			boost::apply_visitor(symbol_list(sym_list), op.left.expr);
			boost::apply_visitor(symbol_list(sym_list), op.right.expr); 
		}
	
		template <unary_op_kind T>
		void operator() (const unary_op<T>& op) const
		{
			boost::apply_visitor(symbol_list(sym_list), op.subject.expr);
		}
	};

	struct dump_expression_ast : public boost::static_visitor<std::string>
	{
		const std::vector<std::string>& remote_syms;

		dump_expression_ast(const std::vector<std::string>& remote_syms_):
			remote_syms(remote_syms_)
		{}

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
			std::ostringstream oss;
			std::vector<std::string>::const_iterator it;
			it = std::find(remote_syms.begin(), remote_syms.end(), s);
			// local scope variable
			if (it == remote_syms.end())
			{
				std::string real_name;
				if (!scope::is_scoped_identifier(s))
					real_name = s;
				else 
					real_name = (scope::decompose(s)).second;
				oss << "a." << real_name;
			} else {
				// remote scope, the value will be available from entry N of the remote_proxy
				size_t i = it - remote_syms.begin();
				oss << "*(r_proxy.at_c<" << i << ">())";
			}

			return oss.str();
		}

		std::string operator() (const function_call& f) const
		{
			std::ostringstream oss;
			oss << f.fName << "::apply(";
			for (size_t i = 0; i < f.args.size(); ++i) {
				oss << boost::apply_visitor(dump_expression_ast(remote_syms), f.args[i].expr);
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
			oss << boost::apply_visitor(dump_expression_ast(remote_syms), e.expr);
			oss << ")";
			return oss.str();
		}

		template <binary_op_kind T>
		std::string operator() (const binary_op<T> & op) const
		{
			std::ostringstream oss;
			oss << "( ";
			oss << "(";
			oss << boost::apply_visitor(dump_expression_ast(remote_syms), op.left.expr);
			oss << ")";
			oss << op.op;
			oss << "(";
			oss << boost::apply_visitor(dump_expression_ast(remote_syms), op.right.expr); 
			oss << ")";
			oss << " )";
			return oss.str();
		}

		template <unary_op_kind T>
		std::string operator() (const unary_op<T>& op) const
		{
			std::ostringstream oss;
			oss << "( ";
			oss << op.op;
			oss << boost::apply_visitor(dump_expression_ast(remote_syms), op.subject.expr);
			oss << ")";
			return oss.str();
		}
	};

	struct is_remote_sym 
	{
		typedef std::string argument_type;
		typedef bool result_type;

		const ability& ctx;

		is_remote_sym(const ability& ctx_): ctx(ctx_) {}

		bool operator() (std::string sym) const
		{
			std::string scope = hyper::compiler::scope::get_scope(sym);
			return (scope != "" && scope != ctx.name());
		}
	};

	struct get_type
	{
		typeId operator() (const std::pair<bool, symbolACL>& p) const 
		{
			assert(p.first == true);
			return p.second.s.t;
		}
	};

	struct build_mpl_vector
	{
		std::string& mpl_vector;
		const typeList& tList;
		
		build_mpl_vector(std::string &mpl_vector_, const typeList& tList_) :
			mpl_vector(mpl_vector_), tList(tList_) {}

		void operator() (typeId t) 
		{
			if (mpl_vector != "") 
				mpl_vector+= ",";
			mpl_vector+= tList.get(t).name;
		}
	};
}

void exec_expression_output::operator() (const expression_ast& e) 
{ 
	const std::string indent = "\t\t";
	const std::string indent_next = indent + "\t";
	const std::string indent_next_next = indent_next + "\t";
	std::vector<std::string> syms;
	boost::apply_visitor(symbol_list(syms), e.expr);

	// Select all the remote_syms
	std::vector<std::string> remote_syms;
	hyper::compiler::copy_if(syms.begin(), syms.end(), 
			std::back_inserter(remote_syms),
			is_remote_sym(ability_context));

	/* Remove duplicate */
	std::sort(remote_syms.begin(), remote_syms.end());
	std::unique(remote_syms.begin(), remote_syms.end());

	/*
	 * if remote_syms is not empty, we need to get their value, using a
	 * remote_proxy_n (see network/proxy.hh)
	 * To construct it, we need to get the type of each variable, using
	 * universe::get_symbol
	 * Note that, at this point, we are sure that the symbol exists
	 */
	std::vector<std::pair<bool, symbolACL> > remote_symbols;
	std::transform(remote_syms.begin(), remote_syms.end(), 
			std::back_inserter(remote_symbols),
			boost::bind(&universe::get_symbol, boost::cref(u),
				_1, boost::cref(ability_context), boost::none));

	bool has_remote_symbols = !remote_symbols.empty();
	std::vector<typeId> remote_sym_type;
	if (has_remote_symbols) {
		/* Generate list of type for each symbol to compute the associed vectorT */
		std::transform(remote_symbols.begin(), remote_symbols.end(),
				std::back_inserter(remote_sym_type),
				get_type());
	}

	std::string mpl_vector;
	std::for_each(remote_sym_type.begin(), remote_sym_type.end(),
			build_mpl_vector(mpl_vector, tList));
	mpl_vector = "boost::mpl::vector<" + mpl_vector + ">";

	std::string class_name = "hyper::" + ability_context.name() + "::ability";

	oss << indent << "void " << base_expr << "condition_" << counter++;
	oss << "(const " << class_name << " & a, " << std::endl;
	oss << indent_next << "bool& res, hyper::model::evaluate_conditions<";
	oss << num_conds << ", " << class_name << ">& cond, size_t i)" << std::endl;
	oss << indent << "{" << std::endl;

	dump_expression_ast dump(remote_syms);
	if (!has_remote_symbols) {
		std::string compute_expr = boost::apply_visitor(dump, e.expr);
		oss << indent_next << "res = " << compute_expr << ";" << std::endl;
	}

	oss << indent_next << "return cond.handle_computation(i);" << std::endl;
	oss << indent << "}" << std::endl;
	oss << std::endl;
}

