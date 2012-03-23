#include <compiler/expression_ast.hh>
#include <compiler/extract_symbols.hh>
#include <compiler/output.hh>
#include <compiler/scope.hh>
#include <compiler/recipe_condition.hh>
#include <compiler/universe.hh>

#include <boost/bind.hpp>

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
			std::string local_symbol = "";

			if (scope::is_scoped_identifier(sym)) {
				std::pair<std::string, std::string> p;
				p = scope::decompose(sym);
				if (p.first == syms.a.name()) 
					local_symbol = p.second;
				else
					syms.remote.insert(sym);
			} else {
				local_symbol = sym;
			}

			if (local_symbol != "") {
				syms.local.insert(local_symbol);
				std::pair< bool, symbolACL > p = syms.a.get_symbol(local_symbol);
				if (p.first && p.second.s.has_updater())
					syms.local_with_updater.insert(local_symbol);
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
	
		void  operator() (const binary_op& op) const
		{
			boost::apply_visitor(extract_syms_helper(syms), op.left.expr);
			boost::apply_visitor(extract_syms_helper(syms), op.right.expr); 
		}
	
		void operator() (const unary_op& op) const
		{
			boost::apply_visitor(extract_syms_helper(syms), op.subject.expr);
		}
	};
	
	struct extract_cond_syms_helper : public boost::static_visitor<void>
	{
		extract_symbols& syms;

		extract_cond_syms_helper(extract_symbols& syms) :
			syms(syms)
		{}

		template <typename T>
		void operator() (const T& ) const {}

		void operator() (const expression_ast& e) const 
		{
			syms.extract(e);
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
			mpl_vector+= tList.get(t).type_name();
		}
	};

	struct add_variable
	{
		std::ostringstream& oss;
		std::string indent;
		
		add_variable(std::ostringstream& oss, const std::string& indent):
			oss(oss), indent(indent) {}

		void operator() (const std::string& sym) const
		{
			std::pair<std::string, std::string> p;
			p = hyper::compiler::scope::decompose(sym);
			oss << indent << "(" << quoted_string(p.first) << ", " << quoted_string(p.second) << ")\n";
		}
	};

	struct add_local_variable
	{
		std::ostringstream& oss;
		std::string indent;
		
		add_local_variable(std::ostringstream& oss, const std::string& indent):
			oss(oss), indent(indent) {}

		void operator() (const std::string& sym) const
		{
			oss << indent << "(" << quoted_string(sym) << ")\n";
		}
	};

	template <typename Iterator>
	std::string generate_local_list(const std::string& base_indent, Iterator begin, Iterator end)
	{
		std::ostringstream oss;
		std::string next_indent = base_indent + "\t";

		oss << base_indent << "boost::assign::list_of<std::string>\n";
		std::for_each(begin, end, add_local_variable(oss, next_indent));
		return oss.str();
	}
}

namespace hyper {
	namespace compiler {
		extract_symbols::extract_symbols(const ability& a) : a(a) {}

		void extract_symbols::extract(const expression_ast& e)
		{
			boost::apply_visitor(extract_syms_helper(*this), e.expr);
		}

		void extract_symbols::extract(const recipe_condition& cond)
		{
			boost::apply_visitor(extract_cond_syms_helper(*this), cond.expr);
		}

		std::string extract_symbols::remote_vector_type_output(const universe& u) const
		{
			/*
			 * if remote is not empty, we need to get their value, using a
			 * remote_values (see network/proxy.hh)
			 * To construct it, we need to get the type of each variable, using
			 * universe::get_symbol
			 * Note that, at this point, we are sure that the symbol exists
			 */
			std::vector<std::pair<bool, symbolACL> > remote_symbols;
			std::transform(remote.begin(), remote.end(), 
					std::back_inserter(remote_symbols),
					boost::bind(&universe::get_symbol, boost::cref(u),
						_1, boost::cref(a), boost::none));

			/* Generate list of type for each symbol to compute the associed vectorT */
			std::vector<typeId> remote_sym_type;
			std::transform(remote_symbols.begin(), remote_symbols.end(),
					std::back_inserter(remote_sym_type),
					get_type());

			std::string mpl_vector;
			std::for_each(remote_sym_type.begin(), remote_sym_type.end(),
					build_mpl_vector(mpl_vector, u.types()));
			return "boost::mpl::vector<" + mpl_vector + ">";
		}

		std::string extract_symbols::remote_list_variables(const std::string& base_indent) const
		{
			std::ostringstream oss;
			std::string next_indent = base_indent + "\t\t\t";
			if (remote.empty()) 
				return "";

			oss << base_indent << "boost::assign::list_of<std::pair<std::string, std::string> >\n";
			std::for_each(remote.begin(), remote.end(), add_variable(oss, next_indent));
			return oss.str();
		}

		std::string extract_symbols::local_list_variables(const std::string& base_indent) const
		{
			if (local.empty()) 
				return "";

			return generate_local_list(base_indent, local.begin(), local.end());
		}

		std::string extract_symbols::local_list_variables_updated(
				const std::string& base_indent) const
																  
		{
			if (local_with_updater.empty())
				return "";

			return generate_local_list(base_indent, local_with_updater.begin(), 
													local_with_updater.end());
		}
	}
}
