#include <boost/bind.hpp>

#include <compiler/ability.hh>
#include <compiler/depends.hh>
#include <compiler/extract_symbols.hh>
#include <compiler/recipe.hh>
#include <compiler/recipe_parser.hh>
#include <compiler/recipe_expression.hh>
#include <compiler/scope.hh>
#include <compiler/task.hh>
#include <compiler/universe.hh>

#include <compiler/condition_output.hh>
#include <compiler/logic_expression_output.hh>
#include <compiler/exec_expression_output.hh>
#include <compiler/output.hh>

using namespace hyper::compiler;

struct validate_expression
{
	bool &b;
	const ability& a;
	const universe &u;

	validate_expression(bool & b_, const ability& a_, const universe& u_) : 
		b(b_), a(a_), u(u_) {}

	void operator() (const expression_ast& e) const
	{
		b = e.is_valid_predicate(a, u, boost::none) && b;
	}
};

struct validate_recipe_expression_ : public boost::static_visitor<bool>
{
	const ability& a;
	const universe& u;
	symbolList& local;

	validate_recipe_expression_(const ability& a_, const universe& u_,
							    symbolList& local_) : 
		a(a_), u(u_), local(local_) {}

	bool operator() (empty) const { assert(false); return false; }

	template <recipe_op_kind kind>
	bool operator() (const recipe_op<kind>& op) const {
		return op.content.is_valid_predicate(a, u, local);
	}

	bool operator() (const expression_ast& e) const {
		return e.is_valid(a, u, local);
	}

	bool operator() (const abort_decl& abort) const {
		std::pair<bool, symbol> p = local.get(abort.identifier);
		if (!p.first) {
			std::cerr << "Error : undefined variable " << abort.identifier;
			std::cerr << " in abort clause " << std::endl;
			return false;
		}

		symbol s = p.second;
		if (s.t != u.types().getId("identifier").second) {
			const type& t = u.types().get(s.t);
			std::cerr << "Error : expected a value of type identifier";
			std::cerr << " for variable " << abort.identifier << " in abort clause ";
			std::cerr << " but got a variable of type " << t.name << std::endl;
			return false;
		}

		return true;
	}

	bool operator() (const let_decl& let) const {
		std::pair<bool, symbolACL> p = a.get_symbol(let.identifier);
		if (p.first) {
			std::cerr << "Error : let declaration of " << let.identifier;
			std::cerr << " will shadow a declaration in " << a.name() << std::endl;
			return false;
		}

		std::pair<bool, symbol> p2 = local.get(let.identifier);
		if (p2.first) {
			std::cerr << "Error : let declaration of " << let.identifier;
			std::cerr << " will shadow a local variable " << std::endl;
			return false;
		}

		bool valid = boost::apply_visitor(validate_recipe_expression_(a,u, local),
				let.bounded.expr);
		if (!valid ) 
			return valid;

		/* Get the type of the expression, we only accept to bind valid
		 * expression, with a non-void return type */
		boost::optional<typeId> t = u.typeOf(a, let.bounded, local);
		if (!t || *t == u.types().getId("void").second)
			return false;

		symbolList::add_result res = local.add(let.identifier, *t);
		return res.first;
	}

	bool operator() (const set_decl& set) const {
		std::string identifier = set.identifier;
		std::string scope = scope::get_scope(identifier);
		if (scope != "" && scope != a.name()) {
			std::cerr << "Can't set the remote value : " << identifier << std::endl;
			std::cerr << "Use make if you want to constraint a remote value" << std::endl;
			return false;
		}

		boost::optional<symbol> s = boost::none;
		if (scope == "") {
			std::pair<bool, symbol> p2 = local.get(set.identifier);
			if (p2.first)
				s = p2.second;
		} else {
			std::pair<std::string, std::string> p = scope::decompose(identifier);
			identifier = p.second;
		}

		std::pair<bool, symbolACL> p = a.get_symbol(identifier);
		if (p.first)
			s = p.second.s;

		if (! s) {
			std::cerr << "Can't find any reference to symbol " << set.identifier;
			std::cerr << " in current context" << std::endl;
			return false;
		}

		bool valid = set.bounded.is_valid(a, u, local);
		if (!valid)
			return valid;

		boost::optional<typeId> t = u.typeOf(a, set.bounded, local);
		if (!t) {
			std::cerr << "Can't compute the type of " << set.bounded << std::endl;
			return false;
		}

		if (s->t != *t) {
			std::cerr << "type of " << set.identifier << " and ";
			std::cerr << set.bounded << " mismatches " << std::endl;
			return false;
		}

		return true;
	}
};



struct validate_recipe_expression
{
	bool &b;
	const ability& a;
	const universe& u;
	symbolList& local;

	validate_recipe_expression(bool & b_, const ability& a_, const universe& u_, 
							   symbolList& local_) :
		b(b_), a(a_), u(u_), local(local_) {}

	void operator() (const recipe_expression& e) const
	{
		b = boost::apply_visitor(validate_recipe_expression_(a, u, local), e.expr) && b;
	}
};

template <typename Iterator> 
bool are_valid_expressions(Iterator begin, Iterator end,
						  const ability& a, const universe& u)
{
	bool b = true;
	std::for_each(begin, end, validate_expression(b, a, u));

	return b;
}

template <typename Iterator>
bool are_valid_recipe_expressions(Iterator begin, Iterator end,
		const ability& a, const universe& u, symbolList& local)
{
	bool b = true;
	std::for_each(begin, end, validate_recipe_expression(b, a, u, local));

	return b;
}

struct dump_recipe_visitor : public boost::static_visitor<std::string>
{
	const universe& u;
	const ability &a;
	const task& t;

	dump_recipe_visitor(const universe & u_, const ability& a_, const task& t_) : 
		u(u_), a(a_), t(t_) {}

	template <typename T> 
	std::string operator() (const T&) const { return ""; }

	std::string operator() (const set_decl& s) const
	{
		std::ostringstream oss;
		std::pair<bool, symbolACL> p = a.get_symbol(s.identifier);
		assert(p.first);
		const typeList& tList = u.types();
		symbol sym = p.second.s;
		type t = tList.get(sym.t);
		std::string tmp_identifier = "_" + s.identifier + "_tmp";

		oss << "\t\t\t" << "{\n";
		oss << "\t\t\t\t" << t.name << " " << tmp_identifier << " = ";
		oss << expression_ast_output(s.bounded) << ";\n";
		oss << "\t\t\t\tboost::shared_lock<boost::shared_mutex> lock(a.mtx);\n";
		oss << "\t\t\t\ta." << s.identifier << " = " << tmp_identifier << ";\n";
		oss << "\t\t\t}\n";

		return oss.str();
	}
};

struct dump_recipe_expression {
	std::ostream& oss;
	const universe &u;
	const ability &a;
	const task& t;

	dump_recipe_expression(std::ostream& oss_, const universe& u_,
						   const ability & a_, const task& t_) : 
		oss(oss_), u(u_), a(a_), t(t_) {}

	void operator() (const recipe_expression& r)
	{
		oss << boost::apply_visitor(dump_recipe_visitor(u, a, t), r.expr);
	}
};

namespace hyper {
	namespace compiler {
		recipe::recipe(const recipe_decl& r_parser, const ability& a, 
						const task& t, const typeList& tList_) :
			name(r_parser.name), pre(r_parser.conds.pre.list),
			post(r_parser.conds.post.list), body(r_parser.body.list),
			context_a(a), context_t(t), tList(tList_), local_symbol(tList) 
		{}

		bool recipe::validate(const universe& u) 
							  
		{
			return are_valid_expressions(pre.begin(), pre.end(), context_a, u) &&
				   are_valid_expressions(post.begin(), post.end(), context_a, u) &&
				   are_valid_recipe_expressions(body.begin(), body.end(), context_a, u, local_symbol);
		}

		void recipe::dump_include(std::ostream& oss, const universe& u) const
		{
			const std::string indent="\t\t";
			const std::string next_indent = indent + "\t";

			std::string exported_name_big = exported_name();
			std::transform(exported_name_big.begin(), exported_name_big.end(), 
						   exported_name_big.begin(), toupper);
			guards g(oss, context_a.name(), exported_name_big + "_HH");

			extract_symbols pre_symbols(context_a.name());
			std::for_each(pre.begin(), pre.end(), 
						  boost::bind(&extract_symbols::extract, &pre_symbols, _1));

			oss << "#include <model/recipe.hh>" << std::endl;
			oss << "#include <model/evaluate_conditions.hh>" << std::endl;

			namespaces n(oss, context_a.name());
			oss << indent << "struct ability;" << std::endl;
			oss << indent << "struct " << exported_name();
			oss << " : public model::recipe {" << std::endl;

			if (!pre.empty()) {
				oss << next_indent << "typedef hyper::model::evaluate_conditions<";
				oss << pre.size() << ", ability, " << pre_symbols.remote_vector_type_output(u);
				oss << " > pre_conditions;" << std::endl; 
				oss << next_indent << "pre_conditions preds;" << std::endl;
			}

			oss << next_indent << "ability &a; " << std::endl;

			oss << next_indent << exported_name();
			oss << "(ability& a_);" << std::endl; 
			oss << next_indent;
			oss << "void async_evaluate_preconditions(model::condition_execution_callback cb);";
			oss << std::endl;
			oss << next_indent << "bool do_execute();\n"; 

			oss << indent << "};" << std::endl;
		}

		void recipe::dump(std::ostream& oss, const universe& u) const
		{
			const std::string indent="\t\t";
			const std::string next_indent = indent + "\t";
			const std::string next_next_indent = next_indent + "\t";

			depends deps;
			void (*f1)(const expression_ast&, const std::string&, depends&) = &add_depends;
			void (*f2)(const recipe_expression&, const std::string&, depends&) = &add_depends;
			std::for_each(pre.begin(), pre.end(),
						  boost::bind(f1 ,_1, boost::cref(context_a.name()),
											 boost::ref(deps)));
			std::for_each(body.begin(), body.end(),
						  boost::bind(f2 ,_1, boost::cref(context_a.name()),
						  boost::ref(deps)));
			
			oss << "#include <" << context_a.name();
			oss << "/ability.hh>" << std::endl;
			oss << "#include <" << context_a.name();
			oss << "/recipes/" << name << ".hh>" << std::endl;

			oss << "#include <boost/assign/list_of.hpp>" << std::endl;

			std::for_each(deps.fun_depends.begin(), 
						  deps.fun_depends.end(), dump_depends(oss, "import.hh"));
			oss << std::endl;

			extract_symbols pre_symbols(context_a.name());
			std::for_each(pre.begin(), pre.end(), 
						  boost::bind(&extract_symbols::extract, &pre_symbols, _1));
			{
			anonymous_namespaces n(oss);
			oss << indent << "using namespace hyper;\n";
			oss << indent << "using namespace hyper::" << context_a.name() << ";\n";

			std::string context_name = "hyper::" + context_a.name() + "::" + exported_name();
			exec_expression_output e_dump(context_a, context_name, oss, "pre_", pre_symbols.remote);
			std::for_each(pre.begin(), pre.end(), e_dump);
			}

			namespaces n(oss, context_a.name());

			/* Generate constructor */
			oss << indent << exported_name() << "::" << exported_name();
			oss << "(hyper::" << context_a.name() << "::ability & a_) :" ;
			oss << "model::recipe(" << quoted_string(name);
			oss << ", a_), a(a_)";
			if (!pre.empty()) {
			oss << ",\n";
			oss << indent << "preds(a_, \n";
			oss << next_indent << "boost::assign::list_of<hyper::" << context_a.name(); 
			oss << "::" << exported_name() << "::pre_conditions::condition>\n";
			generate_condition e_cond(oss, "pre");
			std::for_each(pre.begin(), pre.end(), e_cond);
			oss << pre_symbols.remote_list_variables(next_next_indent);
			oss << indent << ")" << std::endl;
			}
			oss << indent << "{" << std::endl;
			oss << indent << "}\n\n";

			oss << indent << "void " << exported_name();
			oss << "::async_evaluate_preconditions(model::condition_execution_callback cb)";
			oss << std::endl;
			oss << indent << "{" << std::endl;
			if (pre.empty()) 
				oss << next_indent << "cb(hyper::model::conditionV());\n";
			else
				oss << next_indent << "preds.async_compute(cb);\n"; 
			oss << indent << "}" << std::endl;

			oss << indent << "bool " << exported_name();
			oss << "::do_execute()\n";
			oss << indent << "{\n";
			oss << next_indent << "std::cerr << __PRETTY_FUNCTION__ << std::endl;" << std::endl;
			std::for_each(body.begin(), body.end(), 
						  dump_recipe_expression(oss, u, context_a, context_t));
			oss << next_indent << "return true;" << std::endl;
			oss << indent << "}\n";
			oss << std::endl;
		}
	}
}
