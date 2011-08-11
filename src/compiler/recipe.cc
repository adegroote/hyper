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
#include <compiler/extension.hh>
#include <compiler/logic_expression_output.hh>
#include <compiler/exec_expression_output.hh>
#include <compiler/output.hh>

#include <compiler/extract_unused_result.hh>
#include <compiler/recipe_expression_output.hh>

using namespace hyper::compiler;

struct dump_sym {
	std::ostream& oss;
	const typeList& tList;

	dump_sym(std::ostream& oss, const typeList& tList) :
		oss(oss), tList(tList)
	{}

	void operator() (const std::pair<std::string, symbol>& p) const
	{
		const symbol& sym = p.second;
		type t = tList.get(sym.t);
		oss << times(3, "\t") << t.type_name() << " " << sym.name << ";\n";
	}
};

std::string make_local_ctx_structure(const symbolList& syms, const typeList& tList)
{
	std::ostringstream oss; 
	oss << "struct local_context {\n";
	std::for_each(syms.begin(), syms.end(), dump_sym(oss, tList));
	oss << "\t\t};\n";
	return oss.str();
}


struct dump_unused_type {
	std::ostream& oss;
	
	dump_unused_type(std::ostream& oss) : oss(oss) {}

	void operator() (const std::string& s) {
		oss << times(3, "\t") << s << " " << mangle(s) << ";\n";
	}
};

std::string make_unused_ctx_structure(const std::set<std::string>& s)
{
	std::ostringstream oss;
	oss << "struct unused_context {\n";
	std::for_each(s.begin(), s.end(), dump_unused_type(oss));
	oss << "\t\t};\n";
	return oss.str();
}

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

struct valid_unification
{
	bool & b;
	const ability& a;
	const universe& u;
	const symbolList& local;
	boost::optional<std::string> dst;

	valid_unification(bool& b, const ability& a, const universe& u, 
					  const symbolList& local,
					  const boost::optional<std::string>& dst) :
		b(b), a(a), u(u), local(local), dst(dst)
	{}

	void operator() (const unification_expression& p)
	{
		if (!dst) {
			b = false;
			return;
		}

		boost::optional<typeId> t1, t2;
		t1 = u.typeOf(a, p.first, local);
		t2 = u.typeOf(a, p.second, local);

		b = b && t1 && t2 && (*t1 == *t2);
		if (!b)
			return;

		/* Check there are at least one symbol, and that it modifies one
		 * element of dst */
		const std::string *s1, *s2;
		s1 = boost::get<std::string>(&p.first.expr);
		s2 = boost::get<std::string>(&p.second.expr);

		if (!s1 && !s2) {
			b = false;
			std::cerr << "Invalid unification_expression : nor " << s1 << " nor "; 
			std::cerr << s2 << " are symbol " << std::endl;
			return;
		}

		bool b1 = s1 && scope::get_scope(*s1) == *dst;
		bool b2 = s2 && scope::get_scope(*s2) == *dst;

		/* One symbol exactly must be part of the dst */
		if (! b1 ^ b2 ) {
			b = false;
			std::cerr << "Invalid unification_expression : no " << *dst << " symbol ";
			std::cerr << p << std::endl;
			return;
		}
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

	bool operator() (const wait_decl& w) const {
		return w.content.is_valid_predicate(a, u, local);
	}

	template <recipe_op_kind kind>
	bool operator() (const recipe_op<kind>& op) const {
		std::vector<remote_constraint>::const_iterator it;
		bool valid = true;
		for (it = op.content.begin(); it != op.content.end(); ++it) {
			valid &= (it->dst && it->logic_expr.main.is_valid_predicate(a, u, local));
			std::for_each(it->logic_expr.unification_clauses.begin(),
						  it->logic_expr.unification_clauses.end(),
						  valid_unification(valid, a, u, local, it->dst));
		}

		return valid;
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





struct export_ {
	std::ostream& oss;
	const task& t;

	export_(std::ostream& oss, const task& t) : oss(oss), t(t) {}

	void operator() (const std::pair<std::string, symbol>& p) const {
		oss << times(4, "\t") << "a.export_local_variable(";
		oss << quoted_string(exported_symbol(p.first, t)) << ", local_vars." << p.first << ");\n";
	}
};

struct remove_ {
	std::ostream& oss;
	const task& t;

	remove_(std::ostream& oss, const task& t) : oss(oss), t(t) {}

	void operator() (const std::pair<std::string, symbol>& p) const {
		oss << times(4, "\t") << "a.remove_local_variable(";
		oss << quoted_string(exported_symbol(p.first, t)) << ");\n";
	}
};

struct extract_expression_visitor : public boost::static_visitor<void>
{
	std::vector<expression_ast>& list;

	extract_expression_visitor(std::vector<expression_ast>& list) : 
		list(list) {}

	template <typename T> 
	void operator() (const T&) const {}

	void operator() (const let_decl& l) const
	{
		boost::apply_visitor(extract_expression_visitor(list), l.bounded.expr);
	}

	void operator() (const expression_ast& e) const 
	{
		list.push_back(e);
	}

	void operator() (const set_decl& s) const
	{
		list.push_back(s.bounded);
	}

	void operator() (const wait_decl& w) const
	{
		list.push_back(w.content);
	}
};

struct extract_expression {
	std::vector<expression_ast>& list;

	extract_expression(std::vector<expression_ast>& list) : list(list) {}

	void operator() (const recipe_expression& r)
	{
		boost::apply_visitor(extract_expression_visitor(list), r.expr);
	}
};


struct dump_eval_expression {
	std::ostream& oss;
	const universe &u;
	const ability &a;
	const task& t;
	std::string local_data;
	size_t counter;
	const symbolList& local_symbol;

	dump_eval_expression(std::ostream& oss_, const universe& u_,
						   const ability & a_, const task& t_, 
						   const std::string& local_data,
						   const symbolList& local_symbol) : 
		oss(oss_), u(u_), a(a_), t(t_), local_data(local_data), 
		counter(0), local_symbol(local_symbol) 
	{}

	void operator() (const expression_ast& e)
	{
		std::pair<bool, std::string> p = is_tagged_func_call(e, a.name(), u);
		if (p.first)
			return u.get_extension(p.second).dump_eval_expression
				(oss, u, a, t, local_data, local_symbol, counter++, e);

		std::string indent = "\t\t";
		std::string next_indent = "\t" + indent;
		oss << indent << "struct expression_" << counter++ << " {\n";

		/* Compute the necessary arguments */
		extract_symbols syms(a);
		syms.extract(e);
		oss << next_indent << "typedef model::update_variables< ";
		oss << "hyper::" << a.name() << "::ability, ";
		oss << syms.local_with_updater.size() << ", ";
		oss << syms.remote_vector_type_output(u) << " > updater_type;\n";

		/* Compute the return type of the expression */
		const typeList& tList = u.types();
		boost::optional<typeId> id = u.typeOf(a, e, local_symbol); 
		assert(id);
		type t = tList.get(*id);
		oss << next_indent << "typedef " << t.type_name() << " return_type;\n\n";
		oss << next_indent << "return_type operator() (const updater_type& updater, ";
		oss << "const hyper::" << a.name() << "::ability& a,\n";
		oss << next_indent << "\t\t\t\t\t\tconst " << local_data << " & local_vars)\n";
		oss << next_indent << "{\n";
		oss << next_indent << "\treturn " << expression_ast_output(e, syms.remote, local_symbol) << ";\n";
		oss << next_indent << "}\n";
		oss << indent << "};" << std::endl;
	}
};

struct dump_tag_depends
{
	std::ostream& oss;
	const universe& u;
	std::string name;

	dump_tag_depends(std::ostream& oss,
					 const universe& u,
					 const std::string& name) : 
		oss(oss), u(u), name(name) {}

	void operator() (const std::string& tag) {
		oss << "#include <" << name << "/" << tag << "_funcs.hh>\n";
		u.get_extension(tag).recipe_additional_includes(oss, name);
	}
};

struct dump_required_agents
{
	std::ostream& oss;
	std::string name;

	dump_required_agents(std::ostream& oss, const std::string& name) : 
		oss(oss), name(name) {}

	void operator() (const std::string& agent) 
	{
		/* Don't requiere ourself, we are already here ! */
		if (agent == name)
			return;
		oss << times(3, "\t") << "required_agents.insert(";
		oss << quoted_string(agent) << ");\n";
	}
};

struct is_exportable_symbol
{
	const typeList& tList;

	is_exportable_symbol(const typeList& tList) : tList(tList) {}

	bool operator() (const std::pair<std::string, symbol>& p) const
	{
		type t = tList.get(p.second.t);
		return (t.t != opaqueType);
	}
};

struct adapt_expression_to_context_helper : public boost::static_visitor<expression_ast>
{
	const recipe_context_decl::map_type& map;

	adapt_expression_to_context_helper(const recipe_context_decl::map_type& map) : map(map) {}

	template <typename T>
	expression_ast operator() (const T& t) const { return t; }

	expression_ast operator() (const std::string& s) const {
		recipe_context_decl::map_type::const_iterator it;
		it = map.find(s);
		if (it != map.end())
			return boost::apply_visitor(*this, it->second.expr);
		else
			return s;
	}

	expression_ast operator() (const expression_ast& e) const {
		return boost::apply_visitor(*this, e.expr);
	}

	expression_ast operator() (const function_call& f) const {
		function_call res(f);
		for (size_t i = 0; i < res.args.size(); ++i)
			res.args[i] = boost::apply_visitor(*this, res.args[i].expr);
		return res;
	}

	template <unary_op_kind K>
	expression_ast operator() (const unary_op<K>& op) const {
		unary_op<K> res(op);
		res.subject = boost::apply_visitor(*this, op.subject.expr);
		return res;
	}

	template <binary_op_kind K>
	expression_ast operator() (const binary_op<K>& op) const {
		binary_op<K> res(op);
		res.left = boost::apply_visitor(*this, op.left.expr);
		res.right = boost::apply_visitor(*this, op.right.expr);
		return res;
	}
};

struct adapt_expression_to_context {
	const recipe_context_decl::map_type& map;

	adapt_expression_to_context(const recipe_context_decl::map_type& map) : 
		map(map)
	{}

	expression_ast operator() (const expression_ast& ast)
	{
		return boost::apply_visitor(adapt_expression_to_context_helper(map), ast.expr);
	}
};

struct adapt_recipe_expression_to_context_helper : public boost::static_visitor<recipe_expression> {
	const recipe_context_decl::map_type& map;

	adapt_recipe_expression_to_context_helper(const recipe_context_decl::map_type& map) :
		map(map)
	{}

	template <typename T>
	recipe_expression operator() (const T& t) const { return t; }

	recipe_expression operator() (const let_decl& l) const {
		let_decl res(l);
		res.bounded = boost::apply_visitor(*this, l.bounded.expr);
		return res;
	}

	recipe_expression operator() (const set_decl& s) const {
		set_decl res(s);
		res.bounded = adapt_expression_to_context(map)(s.bounded.expr);
		return res;
	}

	recipe_expression operator() (const expression_ast& e) const {
		return adapt_expression_to_context(map)(e.expr);
	}

	recipe_expression operator() (const wait_decl& w) const {
		wait_decl res(w);
		res.content = adapt_expression_to_context(map)(res.content.expr);
		return res;
	}

	template <recipe_op_kind K>
	recipe_expression operator() (const recipe_op<K>& op) const {
		recipe_op<K> res(op);
		for (size_t i = 0; i < res.content.size(); ++i) {
			res.content[i].logic_expr.main =
				adapt_expression_to_context(map)(res.content[i].logic_expr.main.expr);
			res.content[i].compute_dst();
		}
		return res;
	}
};

struct adapt_recipe_expression_to_context {
	const recipe_context_decl::map_type& map;

	adapt_recipe_expression_to_context(const recipe_context_decl::map_type& map) :
		map(map)
	{}

	recipe_expression operator() (const recipe_expression& ast)
	{
		return boost::apply_visitor(adapt_recipe_expression_to_context_helper(map), ast.expr);
	}
};

namespace hyper {
	namespace compiler {
		recipe::recipe(const recipe_decl& r_parser, 
					   const recipe_context_decl& context,	
					   const ability& a, 
					   const task& t, const typeList& tList_) :
			name(r_parser.name), pre(r_parser.conds.pre.list),
			post(r_parser.conds.post.list), body(r_parser.body.list),
			context_a(a), context_t(t), tList(tList_), local_symbol(tList) 
		{
			recipe_context_decl::map_type map = make_name_expression_map(context);
			std::transform(pre.begin(), pre.end(), pre.begin(),
						   adapt_expression_to_context(map));
			std::transform(post.begin(), post.end(), post.begin(),
						   adapt_expression_to_context(map));
			std::transform(body.begin(), body.end(), body.begin(),
						   adapt_recipe_expression_to_context(map));
		}

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

			extract_symbols pre_symbols(context_a);
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
				oss << pre.size() << ", ability, " << pre_symbols.local_with_updater.size() << ", ";
				oss << pre_symbols.remote_vector_type_output(u);
				oss << " > pre_conditions;" << std::endl; 
				oss << next_indent << "pre_conditions preds;" << std::endl;
			}

			oss << next_indent << "ability &a;" << std::endl;
			

			oss << next_indent << exported_name();
			oss << "(ability& a_);" << std::endl; 
			oss << next_indent;
			oss << "void async_evaluate_preconditions(model::condition_execution_callback cb);";
			oss << std::endl;
			oss << "size_t nb_preconditions() const { return " << pre.size() << "; }";
			oss << next_indent << "void do_execute(model::abortable_computation::cb_type cb);\n"; 

			oss << indent << "};" << std::endl;
		}

		void recipe::add_depends(depends& deps, const universe& u) const 
		{
			void (*f1)(const expression_ast&, const std::string&, const universe&, depends&) = &hyper::compiler::add_depends;
			void (*f2)(const recipe_expression&, const std::string&, const universe&, depends&) = &hyper::compiler::add_depends;
			std::for_each(pre.begin(), pre.end(),
						  boost::bind(f1 ,_1, boost::cref(context_a.name()),
											  boost::cref(u),
											  boost::ref(deps)));
			std::for_each(body.begin(), body.end(),
						  boost::bind(f2 ,_1, boost::cref(context_a.name()),
											  boost::cref(u),
											  boost::ref(deps)));
		}

		void recipe::dump(std::ostream& oss, const universe& u) const
		{
			const std::string indent="\t\t";
			const std::string next_indent = indent + "\t";
			const std::string next_next_indent = next_indent + "\t";

			depends deps;
			add_depends(deps, u);
			
			oss << "#include <" << context_a.name() << "/ability.hh>\n"; 
			oss << "#include <" << context_a.name();
			oss << "/recipes/" << name << ".hh>\n"; 
			oss << "#include <model/abortable_function.hh>\n";
			oss << "#include <model/compute_abort_expression.hh>\n";
			oss << "#include <model/compute_ensure_expression.hh>\n";
			oss << "#include <model/compute_expression.hh>\n";
			oss << "#include <model/compute_make_expression.hh>\n";
			oss << "#include <model/compute_wait_expression.hh>\n";
			oss << "#include <boost/assign/list_of.hpp>\n"; 

			std::for_each(deps.fun_depends.begin(), 
						  deps.fun_depends.end(), dump_depends(oss, "import.hh"));
			oss << "\n";

			std::for_each(deps.tag_depends.begin(),
						  deps.tag_depends.end(), dump_tag_depends(oss, u, context_a.name()));
			oss << "\n\n";

			extract_symbols pre_symbols(context_a);
			std::for_each(pre.begin(), pre.end(), 
						  boost::bind(&extract_symbols::extract, &pre_symbols, _1));
			{
			anonymous_namespaces n(oss);
			oss << indent << "using namespace hyper;\n";
			oss << indent << "using namespace hyper::" << context_a.name() << ";\n";

			std::string context_name = "hyper::" + context_a.name() + "::" + exported_name();
			exec_expression_output e_dump(context_a, context_name, oss, "pre_", 
										  pre_symbols.remote, symbolList(u.types()));
			std::for_each(pre.begin(), pre.end(), e_dump);
			} 

			/* Extract expression_ast to execute from the body */
			std::vector<expression_ast> expression_list;
			std::for_each(body.begin(), body.end(), extract_expression(expression_list));

			/* Get the list of instruction executed but not catched by a set or a let */
			std::set<std::string> unused_result_set;
			std::for_each(body.begin(), body.end(), extract_unused_result(unused_result_set, u, context_a, local_symbol));
			{ 
				anonymous_namespaces n(oss);
				oss << indent << "using namespace hyper;\n";
				oss << indent << "using namespace hyper::" << context_a.name() << ";\n";

				oss << indent << make_local_ctx_structure(local_symbol, u.types()) << "\n";
				oss << indent << make_unused_ctx_structure(unused_result_set) << "\n";

				dump_eval_expression e_dump(oss, u, context_a, context_t, "local_context", local_symbol);
				std::for_each(expression_list.begin(), expression_list.end(), e_dump);

				oss << indent << "struct exec_driver : public hyper::model::abortable_computation {\n";
				oss << indent << "\tability& a;\n";
				oss << indent << "\tlocal_context local_vars;\n";
				oss << indent << "\tunused_context unused_res;\n";
				for (size_t i = 0; i < expression_list.size(); ++i) {
					std::pair<bool, std::string> p;
					p = is_tagged_func_call(expression_list[i], context_a.name(), u);
					if (!p.first) {
						oss << indent << "\texpression_" << i << "::updater_type updater" << i << ";\n";
						oss << indent << "\thyper::model::compute_expression<expression_" << i ;
						oss << "> expression_exec" << i << ";\n";
					} else {
						u.get_extension(p.second).generate_expression_caller(oss, i);
					}
				}
				oss << indent << "\texec_driver(ability &a) : a(a)";
				for (size_t i = 0; i < expression_list.size(); ++i) {
					extract_symbols syms(context_a);
					syms.extract(expression_list[i]);
					if (syms.empty()) {
						oss << ",\n";
					} else {
						oss << ",\n" << indent << "\t\tupdater" << i << "(a";
						if (!syms.local_with_updater.empty())
							oss << ",\n\t" << syms.local_list_variables_updated(next_indent);
						if (!syms.remote.empty())
							oss << ",\n\t" << syms.remote_list_variables(next_indent);
						oss << indent << "\t\t),\n";
					}
					oss << indent << "\t\texpression_exec" << i << "(updater" << i << ")";
				}
				oss << "\n" << indent << "\t{\n";
				std::vector<std::pair<std::string, symbol> > exportable_symbol;
				hyper::utils::copy_if(local_symbol.begin(), local_symbol.end(), std::back_inserter(exportable_symbol),
										   is_exportable_symbol(u.types()));
				std::for_each(exportable_symbol.begin(), exportable_symbol.end(), export_(oss, context_t));
				std::for_each(body.begin(), body.end(), 
							  dump_recipe_expression(oss, u, context_a, context_t, local_symbol));
				oss << indent << "\t}\n";
				oss << indent << "\t~exec_driver() {\n";
				std::for_each(exportable_symbol.begin(), exportable_symbol.end(), remove_(oss, context_t));
				oss << indent << "\t}\n";
				oss << indent << "};";

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
			generate_condition e_cond(oss, "pre", context_a, u);
			std::for_each(pre.begin(), pre.end(), e_cond);

			std::string base_type = "pre_conditions::remote_values::remote_vars_conf";

			if (!pre_symbols.local_with_updater.empty())
				oss << ", " << pre_symbols.local_list_variables_updated(next_next_indent);
			if (!pre_symbols.remote.empty()) {
				oss << next_indent << ", " << base_type << "(\n";
				oss << pre_symbols.remote_list_variables(next_next_indent);
				oss << next_indent << ")\n";
			}
			oss << indent << ")" << std::endl;
			}
			oss << indent << "{" << std::endl;
			std::for_each(deps.var_depends.begin(), deps.var_depends.end(),
						  dump_required_agents(oss, context_a.name()));
			oss << indent << "}\n\n";

			oss << indent << "void " << exported_name();
			oss << "::async_evaluate_preconditions(model::condition_execution_callback cb)";
			oss << std::endl;
			oss << indent << "{" << std::endl;
			if (pre.empty()) 
				oss << next_indent << "cb(boost::system::error_code(), hyper::model::conditionV());\n";
			else
				oss << next_indent << "preds.async_compute(cb);\n"; 
			oss << indent << "}" << std::endl;

			oss << indent << "void " << exported_name();
			oss << "::do_execute(hyper::model::abortable_computation::cb_type cb)\n";
			oss << indent << "{\n";
			oss << next_indent << "computation = new exec_driver(a);\n";
			oss << next_indent << "computation->compute(cb);\n";
			oss << indent << "}\n";
			oss << std::endl;
		}
	}
}
