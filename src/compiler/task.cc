#include <compiler/ability.hh>
#include <compiler/scope.hh>
#include <compiler/task.hh>
#include <compiler/types.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>

#include <boost/bind.hpp>

namespace {
	using namespace hyper::compiler;

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
	
	template <binary_op_kind op> struct logic_function {};
	template <> struct logic_function<ADD> { static std::string name() { return "add";}};
	template <> struct logic_function<SUB> { static std::string name() { return "sub";}};
	template <> struct logic_function<MUL> { static std::string name() { return "mul";}};
	template <> struct logic_function<DIV> { static std::string name() { return "div";}};
	template <> struct logic_function<EQ> { static std::string name() { return "equal";}};
	template <> struct logic_function<NEQ>{ static std::string name() { return "not_equal";}};
	template <> struct logic_function<LT> { static std::string name() { return"less";}};
	template <> struct logic_function<LTE>{ static std::string name() { return "less_or_equal";}};
	template <> struct logic_function<GT> { static std::string name() { return"greater";}};
	template <> struct logic_function<GTE>{ static std::string name() { return "geater_or_equal";}};
	template <> struct logic_function<AND>{ static std::string name() { return "and";}};
	template <> struct logic_function<OR> { static std::string name() { return "or";}};

	template <unary_op_kind op> struct logic_unary_function {};
	template <> struct logic_unary_function<NEG> { static std::string name() { return "neg";}};
	
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

	struct expression_dump
	{
		const universe& u;
		const ability& ability_context;
		const task& task_context;
		std::ostream& oss;
		const typeList& tList;
		size_t counter;
		const std::string base_expr;

		expression_dump(const universe& u_,
						const ability& ability_context_, 
						const task& task_context_,
						std::ostream& oss_,
						const typeList& tList_,
						const std::string& base_expr_):
			u(u_),
			ability_context(ability_context_),
			task_context(task_context_),
			oss(oss_), 
			tList(tList_),
			counter(0),
			base_expr(base_expr_)
		{}

		void operator() (const expression_ast& e) 
		{ 
			const std::string indent = "\t\t\t\t\t\t";
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

			std::ostringstream expression_oss;
			expression_oss << base_expr << "expression_" << counter++;
			std::string expression_name = expression_oss.str();
			oss << indent << "struct " << expression_name;
			oss << " : public model::expression<" << mpl_vector << " > {" << std::endl;
			oss << indent_next << "const ability &a;" << std::endl;
			if (has_remote_symbols)
			{
				oss << indent_next << expression_name;
				oss << "(const ability& a_, boost::asio::io_service &io_s,";
				oss << "const model::expression< " << mpl_vector << " >::arg_list& l,";
				oss << "network::name_client& r) : " << std::endl;

				oss << indent_next << "a(a_), model::expression< " << mpl_vector << " >(";
				oss << "io_s, l, r) {}" << std::endl;
			}

			if (!has_remote_symbols) {
				oss << indent_next << expression_name << "(const ability& a_) : a(a_) {}";
				oss << std::endl;
			}

			dump_expression_ast dump(remote_syms);
			oss << indent_next << "bool real_compute() const {" << std::endl;
			std::string compute_expr = boost::apply_visitor(dump, e.expr);
			oss << indent_next_next << "return (" << compute_expr << ");" << std::endl;
			oss << indent_next << "}" << std::endl;
			oss << indent << "};" << std::endl;
		}
	};
}

namespace hyper {
	namespace compiler {

			
			void task::dump(std::ostream& oss, const typeList& tList, 
							  const universe&u, const ability& context) const
			{
				const std::string indent="\t\t\t\t\t";
				const std::string next_indent = indent + "\t";
				oss << indent << "struct " << name << " {" << std::endl;
				oss << next_indent << "const ability& a;" << std::endl;
				oss << next_indent << name << "(const ability& a_) : a(a_) {}" << std::endl;
				{
				expression_dump e_dump(u, context, *this, oss, tList, "pre_");
				std::for_each(pre.begin(), pre.end(), e_dump);
				}
				{
				expression_dump e_dump(u, context, *this, oss, tList, "post_");
				std::for_each(post.begin(), post.end(), e_dump);
				}
				oss << indent << "};" << std::endl;
			}

			std::ostream& operator << (std::ostream& oss, const task& t)
			{
				oss << "Task " << t.name << std::endl;
				oss << "Pre : " << std::endl;
				std::copy(t.pre.begin(), t.pre.end(), 
						std::ostream_iterator<expression_ast>( oss, "\n" ));
				oss << std::endl << "Post : " << std::endl;
				std::copy(t.post.begin(), t.post.end(),
						std::ostream_iterator<expression_ast>( oss, "\n" ));
				return oss;
			}
	}
}
