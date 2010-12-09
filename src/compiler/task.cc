#include <compiler/ability.hh>
#include <compiler/depends.hh>
#include <compiler/output.hh>
#include <compiler/scope.hh>
#include <compiler/task.hh>
#include <compiler/task_parser.hh>
#include <compiler/types.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>

#include <boost/bind.hpp>

namespace {
	using namespace hyper::compiler;

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

	std::string generate_logic_expression(const expression_ast& e)
	{
		return boost::apply_visitor(dump_logic_expression_ast(), e.expr);
	}
	
	struct generate_logic_fact {
		std::string name;
		std::ostream &oss;

		generate_logic_fact(const std::string& name_, std::ostream& oss_):
			name(name_), oss(oss_)
		{}

		void operator() (const expression_ast& e) const
		{
			oss << "\t\t\ta_.logic().engine.add_fact(";
			oss << quoted_string(generate_logic_expression(e));
			oss << ", " << quoted_string(name) <<  ");" << std::endl;
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
		size_t num_conds;
		const std::string base_expr;

		expression_dump(const universe& u_,
						const ability& ability_context_, 
						const task& task_context_,
						std::ostream& oss_,
						const typeList& tList_,
						size_t num_conds_,
						const std::string& base_expr_):
			u(u_),
			ability_context(ability_context_),
			task_context(task_context_),
			oss(oss_), 
			tList(tList_),
			counter(0),
			num_conds(num_conds_),
			base_expr(base_expr_)
		{}

		void operator() (const expression_ast& e) 
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
	};

	struct generate_precond {
		std::ostream& oss;
		size_t counter;

		generate_precond(std::ostream& oss_) : oss(oss_), counter(0) {}

		void operator() (const expression_ast& e) const
		{
			oss << "\t\t\t\t\t(pre_condition_" << counter;
			oss << ", " << quoted_string(generate_logic_expression(e)) << ")";
			oss << std::endl;
		}
	};

	struct cond_validate
	{
		bool& res;
		const ability& ab;
		const universe& u;

		cond_validate(bool &res_, const ability& ab_, const universe& u_):
			res(res_), ab(ab_), u(u_)
		{}

		void operator() (const expression_ast& cond) 
		{
			res = cond.is_valid_predicate(ab, u, boost::none) && res;
		}
	};
}

namespace hyper {
	namespace compiler {

			task::task(const task_decl& decl, const ability& a_, const typeList& tList_):
				name(decl.name), pre(decl.conds.pre.list), post(decl.conds.post.list),
				ability_context(a_), tList(tList_) 
			{}

			bool task::validate(const universe& u) const
			{
				bool res = true;
				{
				cond_validate valid(res, ability_context, u);
				std::for_each(pre.begin(), pre.end(), valid); 
				res = valid.res && res;
				}
				{
				cond_validate valid(res, ability_context, u);
				std::for_each(post.begin(), post.end(), valid); 
				res = valid.res && res;
				}
				return res;
			}

			void task::dump_include(std::ostream& oss, const universe& u) const
			{
				const std::string indent="\t\t";
				const std::string next_indent = indent + "\t";

				std::string exported_name_big = exported_name();
				std::transform(exported_name_big.begin(), exported_name_big.end(), 
							   exported_name_big.begin(), toupper);
				guards g(oss, ability_context.name(), exported_name_big + "_HH");

				oss << "#include <model/task.hh>" << std::endl;

				namespaces n(oss, ability_context.name());
				oss << indent << "struct ability;" << std::endl;
				oss << indent << "struct " << exported_name();
				oss << " : public model::task {" << std::endl;

				oss << next_indent << "hyper::model::evaluate_conditions<";
				oss << pre.size() << ", ability> preds;" << std::endl; 

				oss << next_indent << exported_name();
				oss << "(ability& a_);" << std::endl; 
				oss << next_indent;
				oss << "void async_evaluate_preconditions(model::condition_execution_callback cb);";
				oss << std::endl;

				oss << indent << "};" << std::endl;
			}

			void task::dump(std::ostream& oss, const universe& u) const
			{
				const std::string indent="\t\t";
				const std::string next_indent = indent + "\t";

				depends deps;
				void (*f)(const expression_ast&, const std::string&, depends&) = &add_depends;
				std::for_each(pre.begin(), pre.end(),
							  boost::bind(f ,_1, boost::cref(ability_context.name()),
												 boost::ref(deps)));
				std::for_each(post.begin(), post.end(),
							  boost::bind(f ,_1, boost::cref(ability_context.name()),
												 boost::ref(deps)));
				
				oss << "#include <" << ability_context.name();
				oss << "/ability.hh>" << std::endl;
				oss << "#include <" << ability_context.name();
				oss << "/tasks/" << name << ".hh>" << std::endl;

				oss << "#include <boost/assign/list_of.hpp>" << std::endl;

				std::for_each(deps.fun_depends.begin(), 
							  deps.fun_depends.end(), dump_depends(oss, "import.hh"));
				oss << std::endl;

				{
				anonymous_namespaces n(oss);
				expression_dump e_dump(u, ability_context, *this, oss, tList, pre.size(), "pre_");
				std::for_each(pre.begin(), pre.end(), e_dump);
				}

				namespaces n(oss, ability_context.name());

				/* Generate constructor */
				oss << indent << exported_name() << "::" << exported_name();
				oss << "(hyper::" << ability_context.name() << "::ability & a_) :" ;
				oss << "model::task(a_, " << quoted_string(name);
				oss << "),\n";
				oss << indent << "preds(a_, boost::assign::list_of<hyper::model::evaluate_conditions<";
				oss << pre.size() << ",ability>::condition>";
				generate_precond e_cond(oss);
				std::for_each(pre.begin(), pre.end(), e_cond);
				oss << indent << ")" << std::endl;
				oss << indent << "{" << std::endl;

				generate_logic_fact e_fact(name, oss);
				std::for_each(post.begin(), post.end(), e_fact);

				oss << indent << "}\n\n";

				oss << indent << "void " << exported_name();
				oss << "::async_evaluate_preconditions(model::condition_execution_callback cb)";
				oss << std::endl;
				oss << indent << "{" << std::endl;
				oss << next_indent << "preds.async_compute(cb);\n"; 
				oss << indent << "}" << std::endl;
#if 0
				{
				expression_dump e_dump(u, ability_context, *this, oss, tList, "post_");
				std::for_each(post.begin(), post.end(), e_dump);
				}
				oss << indent << "};" << std::endl;
#endif
			}

			std::ostream& operator << (std::ostream& oss, const task& t)
			{
				oss << "Task " << t.get_name() << std::endl;
				oss << "Pre : " << std::endl;
				std::copy(t.pre_begin(), t.pre_end(), 
						std::ostream_iterator<expression_ast>( oss, "\n" ));
				oss << std::endl << "Post : " << std::endl;
				std::copy(t.post_begin(), t.post_end(),
						std::ostream_iterator<expression_ast>( oss, "\n" ));
				return oss;
			}
	}
}
