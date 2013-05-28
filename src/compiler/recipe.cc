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
#include <compiler/recipe_context.hh>

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

struct validate_condition
{
	const ability& a;
	const universe &u;

	validate_condition(const ability& a_, const universe& u_) : 
		a(a_), u(u_) {}

	bool operator() (const recipe_condition& e) const
	{
		return e.is_valid(a, u);
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

template <typename Iterator>
bool are_valid_recipe_expressions(Iterator begin, Iterator end,
		const ability& a, const universe& u, symbolList& local);

struct validate_recipe_expression_ : public boost::static_visitor<bool>
{
	const ability& a;
	const universe& u;
	symbolList& local;

	validate_recipe_expression_(const ability& a_, const universe& u_,
							    symbolList& local_) : 
		a(a_), u(u_), local(local_) {}

	bool operator() (empty) const { assert(false); return false; }

	// check there is a least one element, that all element are valid, and that
	// the last part is really a predicate.
	template <observer_op_kind K>
	bool operator() (const observer_op<K>& w) const {
		if (w.content.size() == 0) return false;

		if (!are_valid_recipe_expressions(
				w.content.begin(), w.content.end(),
				a, u, local))
			return false;

		const expression_ast* e= boost::get<expression_ast>(& w.content.back().expr);
		return (e && e->is_valid_predicate(a, u, local));
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

	bool operator() (const while_decl& while_d) const {
		bool valid = while_d.condition.is_valid_predicate(a, u, local);
		if (!valid)
			return valid;

		return are_valid_recipe_expressions(
				while_d.body.begin(), while_d.body.end(),
				a, u, local);
	}
};

struct validate_recipe_expression
{
	const ability& a;
	const universe& u;
	symbolList& local;

	validate_recipe_expression(const ability& a_, const universe& u_, 
							   symbolList& local_) :
		a(a_), u(u_), local(local_) {}

	bool operator() (const recipe_expression& e) const
	{
		return boost::apply_visitor(validate_recipe_expression_(a, u, local), e.expr);
	}
};

template <typename Iterator> 
bool are_valid_conditions(Iterator begin, Iterator end,
						  const ability& a, const universe& u)
{
	return hyper::utils::all(begin, end, validate_condition(a, u));
}

template <typename Iterator>
bool are_valid_recipe_expressions(Iterator begin, Iterator end,
		const ability& a, const universe& u, symbolList& local)
{
	return hyper::utils::all(begin, end, validate_recipe_expression(a, u, local));
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

struct extract_expression {
	std::vector<expression_ast>& list;

	extract_expression(std::vector<expression_ast>& list) : list(list) {}
	void operator() (const recipe_expression& r);
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

	template <observer_op_kind K>
	void operator() (const observer_op<K>& w) const
	{
		extract_expression extract(list);
		std::for_each(w.content.begin(), w.content.end(), extract);
	}

	void operator() (const while_decl& w) const
	{
		list.push_back(w.condition);
		extract_expression extract(list);
		std::for_each(w.body.begin(), w.body.end(), extract);
	}
};


void extract_expression::operator() (const recipe_expression& r)
{
	boost::apply_visitor(extract_expression_visitor(list), r.expr);
}

struct add_inner_var_visitor : public boost::static_visitor<void>
{
	symbolList& local; 
	size_t& counter;

	add_inner_var_visitor(symbolList& local, size_t& counter) :
		local(local), counter(counter)
	{}

	template <typename T>
	void operator() (const T&) const {}

	void operator() (const observer_op<ASSERT>&) const 
	{
		std::ostringstream oss; 
		oss << "__hyper_assert_" << counter;
		local.add(oss.str(), "bool");
	}
};

struct add_inner_var {
	symbolList& local; 
	size_t& counter;

	add_inner_var(symbolList& local, size_t& counter) :
		local(local), counter(counter)
	{}
	
	void operator() (const recipe_expression& r)
	{
		boost::apply_visitor(add_inner_var_visitor(local, counter), r.expr);
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

struct constraint_domain_helper : public boost::static_visitor<std::string>
{
	const ability& a;
	const universe& u;

	constraint_domain_helper(const ability& a, const universe& u) : a(a), u(u) {}

	template <typename T>
	std::string operator() (const T&) const { return ""; }

	std::string operator() (const let_decl& l) const
	{
		return boost::apply_visitor(*this, l.bounded.expr);
	}

	template <recipe_op_kind kind>
	std::string operator() (const recipe_op<kind>&r ) const
	{
		std::ostringstream oss;
		oss << "hyper::logic::expression(a_.logic().generate(";
		oss <<  generate_logic_expression(r.content[0].logic_expr.main, a, u, false);
		oss << "))";
		
		return oss.str();
	}
};

struct dump_constraint_domain
{
	std::ostream& oss;
	const ability& a;
	const universe& u;

	dump_constraint_domain(std::ostream& oss, const ability& a,
						   const universe& u) : oss(oss), a(a), u(u)
	{}

	void operator() (const recipe_expression& expr)
	{
		std::string s = boost::apply_visitor(constraint_domain_helper(a, u), expr.expr);
		if (s != "")
			oss << times(3, "\t") << "constraint_domain.insert(" << s << ");\n";
	}
};

struct is_exportable_symbol
{
	const typeList& tList;

	is_exportable_symbol(const typeList& tList) : tList(tList) {}

	bool operator() (const std::pair<std::string, symbol>& p) const
	{
		type t = tList.get(p.second.t);
		return (t.t != opaqueType and t.t != noType);
	}
};


struct generate_recipe_condition_helper : public boost::static_visitor<void>
{
	std::ostream& oss;
	const generate_condition& gen;

	generate_recipe_condition_helper(std::ostream& oss, const generate_condition& gen) : 
		oss(oss), gen(gen)
	{}

	void operator() (const empty&) const {}

	void operator() (const expression_ast& e) const {
		return gen(e);
	}

	void operator() (const last_error& error) const {
		// XXX TODO
		(void) error;
	}
};

struct generate_recipe_condition {
	std::ostream& oss;
	generate_condition gen;


	generate_recipe_condition(std::ostream& oss, const std::string& base,
			const ability& a, 
			boost::optional<const universe&> u = boost::none) : 
		oss(oss), gen(oss, base, a, u)
	{}

	void operator() (const recipe_condition& cond) const
	{
		return boost::apply_visitor(generate_recipe_condition_helper(oss, gen), cond.expr);
	}
};

struct exec_condition_output_helper : public boost::static_visitor<void>
{
	exec_expression_output& exec_output;

	exec_condition_output_helper(exec_expression_output& exec_output) :
		exec_output(exec_output)
	{}

	void operator() (const empty&) const {}

	void operator() (const expression_ast& e) const {
		return exec_output(e);
	}

	void operator() (const last_error& error) const {
		// XXX TODO
		(void) error;
	}
};

struct exec_condition_output
{
	exec_expression_output exec_output;

	exec_condition_output(
			const ability& ability_context, 
			const std::string& context_name,
			std::ostream& oss,
			const std::string& base_expr,
			const std::set<std::string>& remote_symbols,
			const symbolList& local_symbols):
		exec_output(ability_context, context_name, oss, base_expr, remote_symbols, local_symbols)
	{}

	void operator() (const recipe_condition& cond)
	{
		return boost::apply_visitor(exec_condition_output_helper(exec_output), cond.expr);
	}
};

struct is_last_error_helper : public boost::static_visitor<bool>
{
	template <typename T>
	bool operator() (const T&) const { return false; }

	bool operator() (const last_error& ) const { return true; }
};

struct is_last_error {
	bool operator() (const recipe_condition& cond) const {
		return boost::apply_visitor(is_last_error_helper(), cond.expr);
	}
};

struct is_expression_helper : public boost::static_visitor<bool>
{
	template <typename T>
	bool operator() (const T& ) const { return false; }

	bool operator() (const expression_ast&) const { return true; }
};

struct is_expression_ast {
	bool operator() (const recipe_condition& cond) const { 
		return boost::apply_visitor(is_expression_helper(), cond.expr);
	}
};

template <typename It>
boost::optional<expression_ast> extract_last_error(It begin, It end)
{
	It found = std::find_if(begin, end, is_last_error());
	if (found == end)
		return boost::none;

	last_error last = boost::get<last_error>(found->expr);
	return last.error;
}

namespace hyper {
	namespace compiler {
		recipe::recipe(const recipe_decl& r_parser, 
					   const recipe_context_decl& context,	
					   const ability& a, 
					   const task& t, const typeList& tList_) :
			name(r_parser.name), 
			pre(r_parser.conds.pre),
			post(r_parser.conds.post),
			body(r_parser.body.list),
			prefer((r_parser.preference_level ? *r_parser.preference_level : 0)),
			context_a(a), context_t(t), tList(tList_), local_symbol(tList) 
		{
			recipe_context_decl::map_type map = make_name_expression_map(context);
			map_fun_def map_fun = make_map_fun_def(context);

			apply_fun_body(body, map_fun);

			std::transform(pre.begin(), pre.end(), pre.begin(),
						   adapt_recipe_condition_to_context(map));
			std::transform(post.begin(), post.end(), post.begin(),
						   adapt_recipe_condition_to_context(map));
			std::transform(body.begin(), body.end(), body.begin(),
						   adapt_recipe_expression_to_context(map));

			has_preconds = (std::count_if(pre.begin(), pre.end(), is_expression_ast()) != 0);

			if (r_parser.end) {
				end = r_parser.end->list;
				apply_fun_body(end, map_fun);
				std::transform(end.begin(), end.end(), end.begin(),
							  adapt_recipe_expression_to_context(map));
			}
		}

		bool recipe::validate(const universe& u) 
							  
		{
			return are_valid_conditions(pre.begin(), pre.end(), context_a, u) &&
				   are_valid_conditions(post.begin(), post.end(), context_a, u) &&
				   are_valid_recipe_expressions(body.begin(), body.end(), context_a, u, local_symbol) &&
				   are_valid_recipe_expressions(end.begin(), end.end(), context_a, u, local_symbol);
		}

		void recipe::dump_include(std::ostream& oss, const universe& u) const
		{
			const std::string indent="\t\t";
			const std::string next_indent = indent + "\t";

			std::string exported_name_big = exported_name();
			std::transform(exported_name_big.begin(), exported_name_big.end(), 
						   exported_name_big.begin(), toupper);
			guards g(oss, context_a.name(), exported_name_big + "_HH");

			void (extract_symbols::*f) (const recipe_condition&) = &extract_symbols::extract;

			extract_symbols pre_symbols(context_a);
			std::for_each(pre.begin(), pre.end(), 
						  boost::bind(f, &pre_symbols, _1));


			oss << "#include <model/task.hh>" << std::endl;
			oss << "#include <model/recipe.hh>" << std::endl;
			oss << "#include <model/evaluate_conditions.hh>" << std::endl;

			namespaces n(oss, context_a.name());
			oss << indent << "struct ability;" << std::endl;
			oss << indent << "struct " << exported_name();
			oss << " : public model::recipe {" << std::endl;

			size_t nb_exec_precondition = std::count_if(pre.begin(), pre.end(), is_expression_ast());

			if (has_preconds) {
				oss << next_indent << "typedef hyper::model::evaluate_conditions<";
				oss << nb_exec_precondition << ", ability, " << pre_symbols.local_with_updater.size() << ", ";
				oss << pre_symbols.remote_vector_type_output(u);
				oss << " > pre_conditions;" << std::endl; 
				oss << next_indent << "pre_conditions preds;" << std::endl;
			}

			oss << next_indent << "ability &a;" << std::endl;
			

			oss << next_indent << exported_name();
			oss << "(ability& a_, hyper::model::task& t_);" << std::endl; 
			oss << next_indent;
			oss << "void async_evaluate_preconditions(model::condition_execution_callback cb);";
			oss << std::endl;
			oss << next_indent << "void do_execute(model::abortable_computation::cb_type cb, bool);\n"; 
			oss << next_indent << "void do_end(model::abortable_computation::cb_type cb);\n"; 

			oss << indent << "};" << std::endl;
		}

		void recipe::add_depends(depends& deps, const universe& u) const 
		{
			void (*f1)(const recipe_condition&, const std::string&, const universe&, depends&) = &hyper::compiler::add_depends;
			void (*f2)(const recipe_expression&, const std::string&, 
					   const universe&, depends&, const symbolList&) = &hyper::compiler::add_depends;
			std::for_each(pre.begin(), pre.end(),
						  boost::bind(f1 ,_1, boost::cref(context_a.name()),
											  boost::cref(u),
											  boost::ref(deps)));
			std::for_each(body.begin(), body.end(),
						  boost::bind(f2 ,_1, boost::cref(context_a.name()),
											  boost::cref(u),
											  boost::ref(deps),
											  boost::cref(local_symbol)
											  ));
		}


		void recipe::dump(std::ostream& oss, const universe& u) 
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
			oss << "#include <model/compute_assert_expression.hh>\n";
			oss << "#include <model/compute_make_expression.hh>\n";
			oss << "#include <model/compute_wait_expression.hh>\n";
			oss << "#include <model/compute_while_expression.hh>\n";
			oss << "#include <model/logic_layer.hh>\n";
			oss << "#include <boost/assign/list_of.hpp>\n"; 

			std::for_each(deps.fun_depends.begin(), 
						  deps.fun_depends.end(), dump_depends(oss, "import.hh"));
			oss << "\n";

			std::for_each(deps.tag_depends.begin(),
						  deps.tag_depends.end(), dump_tag_depends(oss, u, context_a.name()));
			oss << "\n\n";

			extract_symbols pre_symbols(context_a);
			void (extract_symbols::*f) (const recipe_condition&) = &extract_symbols::extract;
			std::for_each(pre.begin(), pre.end(), 
						  boost::bind(f, &pre_symbols, _1));
			{
			anonymous_namespaces n(oss);
			oss << indent << "using namespace hyper;\n";
			oss << indent << "using namespace hyper::" << context_a.name() << ";\n";

			std::string context_name = "hyper::" + context_a.name() + "::" + exported_name();
			exec_condition_output e_dump(context_a, context_name, oss, "pre_", 
										  pre_symbols.remote, symbolList(u.types()));
			std::for_each(pre.begin(), pre.end(), e_dump);
			} 

			/* add variable necessary for computation in local_vars */
			size_t var_count = 0;
			std::for_each(body.begin(), body.end(), add_inner_var(local_symbol, var_count));

			/* Extract expression_ast to execute from the body */
			std::vector<expression_ast> expression_list;
			std::for_each(body.begin(), body.end(), extract_expression(expression_list));
			size_t nb_expr_body = expression_list.size();

			std::for_each(end.begin(), end.end(), extract_expression(expression_list));

			/* Get the list of instruction executed but not catched by a set or a let */
			std::set<std::string> unused_result_set;
			std::for_each(body.begin(), body.end(), extract_unused_result(unused_result_set, u, context_a, local_symbol));
			std::for_each(end.begin(), end.end(), extract_unused_result(unused_result_set, u, context_a, local_symbol));
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
				for (size_t i = 0; i < nb_expr_body; ++i) {
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
				for (size_t i = 0; i < nb_expr_body; ++i) {
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
				oss << indent << "};\n";


				oss << indent << "struct end_driver : public hyper::model::abortable_computation {\n";
				oss << indent << "\tability& a;\n";
				oss << indent << "\tlocal_context local_vars;\n";
				oss << indent << "\tunused_context unused_res;\n";
				for (size_t i = nb_expr_body; i < expression_list.size(); ++i) {
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
				oss << indent << "\tend_driver(ability &a) : a(a)";
				for (size_t i = nb_expr_body; i < expression_list.size(); ++i) {
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
				std::for_each(end.begin(), end.end(), 
						dump_recipe_expression(oss, u, context_a, context_t, local_symbol, nb_expr_body));
				oss << indent << "\t}\n";
				oss << indent << "\t~end_driver() {}\n";
				oss << indent << "};";


			}

			namespaces n(oss, context_a.name());

			boost::optional<expression_ast> last_error = extract_last_error(pre.begin(), pre.end());
			/* Generate constructor */
			oss << indent << exported_name() << "::" << exported_name();
			oss << "(hyper::" << context_a.name() << "::ability & a_, hyper::model::task& t_) :" ;
			oss << "model::recipe(" << quoted_string(name) << ", a_, t_";
			if (last_error) {
				oss << ", hyper::logic::expression(a_.logic().generate(";
				oss << generate_logic_expression(*last_error, context_a, u, false);
				oss << "))";
			}
			oss << "), a(a_)";
			if (has_preconds) {
			oss << ",\n";
			oss << indent << "preds(a_, \n";
			oss << next_indent << "boost::assign::list_of<hyper::" << context_a.name(); 
			oss << "::" << exported_name() << "::pre_conditions::condition>\n";
			generate_recipe_condition e_cond(oss, "pre", context_a, u);
			std::for_each(pre.begin(), pre.end(), e_cond);

			std::string base_type = "pre_conditions::remote_values::remote_vars_conf";

			if (!pre_symbols.local_with_updater.empty()) {
				oss << ", boost::array<std::string, " << pre_symbols.local_with_updater.size();
				oss <<">(" << pre_symbols.local_list_variables_updated(next_next_indent) << ")";
			}
			if (!pre_symbols.remote.empty()) {
				oss << next_indent << ", " << base_type << "(\n";
				oss << pre_symbols.remote_list_variables(next_next_indent);
				oss << next_indent << ")\n";
			}
			oss << indent << ")" << std::endl;
			}
			oss << indent << "{" << std::endl;

			size_t nb_exec_precondition = std::count_if(pre.begin(), pre.end(), is_expression_ast());
			oss << next_indent << "nb_preconditions_ = " << nb_exec_precondition << ";\n";
			oss << next_indent << "has_end_handler = " << (end.empty() ? "false" : "true") << ";\n";
			oss << next_indent << "prefer_ = " << prefer << ";\n";
			std::for_each(deps.var_depends.begin(), deps.var_depends.end(),
						  dump_required_agents(oss, context_a.name()));
			std::for_each(body.begin(), body.end(), dump_constraint_domain(oss, context_a, u));
			oss << indent << "}\n\n";

			oss << indent << "void " << exported_name();
			oss << "::async_evaluate_preconditions(model::condition_execution_callback cb)";
			oss << std::endl;
			oss << indent << "{" << std::endl;
			if (!has_preconds) 
				oss << next_indent << "cb(boost::system::error_code(), hyper::model::conditionV());\n";
			else
				oss << next_indent << "preds.async_compute(cb);\n"; 
			oss << indent << "}" << std::endl;

			oss << indent << "void " << exported_name();
			oss << "::do_execute(hyper::model::abortable_computation::cb_type cb, bool must_pause)\n";
			oss << indent << "{\n";
			oss << next_indent << "computation = new exec_driver(a);\n";
			oss << next_indent << "if (must_pause) computation->pause();\n";
			oss << next_indent << "computation->compute(cb);\n";
			oss << indent << "}\n";
			oss << std::endl;

			oss << indent << "void " << exported_name();
			oss << "::do_end(hyper::model::abortable_computation::cb_type cb)\n";
			oss << indent << "{\n";
			oss << next_indent << "end_handler = new end_driver(a);\n";
			oss << next_indent << "end_handler->compute(cb);\n";
			oss << indent << "}\n";
			oss << std::endl;
		}
	}
}
