#include <compiler/expression_ast.hh>
#include <compiler/extract_symbols.hh>
#include <compiler/scope.hh>
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

namespace hyper {
	namespace compiler {
		extract_symbols::extract_symbols(const std::string& context) : context(context) {}

		void extract_symbols::extract(const expression_ast& e)
		{
			boost::apply_visitor(extract_syms_helper(*this), e.expr);
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
						_1, boost::cref(u.get_ability(context)), boost::none));

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
	}
}
