#include <fstream>
#include <set>

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/make_shared.hpp>

#include <compiler/ability_parser.hh>
#include <compiler/extension.hh>
#include <compiler/logic_expression_output.hh>
#include <compiler/output.hh>
#include <compiler/universe.hh>
#include <compiler/scope.hh>
#include <compiler/task_parser.hh>

using namespace hyper::compiler;

universe::universe() : tList(), fList(tList)
{
	// Add basic native types
	
	tList.add("bool", boolType);
	tList.add("double", doubleType);
	tList.add("int", intType);
	tList.add("string", stringType);
	tList.add("void", noType);
	tList.add("identifier", structType);
}

struct sym_add_scope 
{
	std::string scope_;

	sym_add_scope(const std::string& s) : scope_(s) {};
	symbol_decl operator() (const symbol_decl &s)
	{
		symbol_decl res = s;
		res.typeName = scope::add_scope(scope_, res.typeName);
		return res;
	}
};

struct type_add_scope_visitor : public boost::static_visitor<type_decl>
{
	std::string scope_;
	type_add_scope_visitor(const std::string& s)  : 
		scope_(s) {};

	type_decl operator()(const newtype_decl& decl)
	{
		newtype_decl res = decl;
		res.oldname = scope::add_scope(scope_, res.oldname);
		res.newname = scope::add_scope(scope_, res.newname);
		return res;
	}

	type_decl operator()(const struct_decl& decl)
	{
		struct_decl res;
		sym_add_scope sym_scope(scope_);
		res.name = scope::add_scope(scope_, decl.name);
		res.vars.l.resize(decl.vars.l.size());
		std::transform(decl.vars.l.begin(), decl.vars.l.end(),
						res.vars.l.begin(), sym_scope);
		return res;
	}

	type_decl operator()(const opaque_decl& decl)
	{
		opaque_decl res;
		res.name = scope::add_scope(scope_, decl.name);
		return res;
	}
};

struct type_add_scope
{
	std::string scope_;

	type_add_scope(const std::string& s) : scope_(s) {};

	type_decl operator()(const type_decl& decl)
	{
		type_add_scope_visitor v(scope_);
		return boost::apply_visitor(v, decl);
	}
};

struct functions_def_add_scope {
	std::string scope_;

	functions_def_add_scope(const std::string& s) : scope_(s) {};

	function_decl operator()(const function_decl& decl)
	{
		function_decl res = decl;
		res.fName = scope::add_scope(scope_, res.fName);
		res.returnName = scope::add_scope(scope_, res.returnName);
		
		std::transform(res.argsName.begin(), res.argsName.end(),
					   res.argsName.begin(), boost::bind(&scope::add_scope, scope_, _1));
		return res;
	}
};

struct rule_add_scope {
	std::string scope;

	rule_add_scope(const std::string& s) : scope(s) {};

	rule_decl operator()(const rule_decl& decl)
	{
		rule_decl res = decl;
		res.name = scope::add_scope(scope, res.name);
		return res;
	}
};

struct type_diagnostic_visitor : public boost::static_visitor<void>
{
	// XXX improve type_diagnostic :)
	const type_decl &decl;

	type_diagnostic_visitor(const type_decl& decl_) : decl(decl_) {};

	void operator() (const typeList::native_decl_error& error) const
	{
		(void) error;
	}

	void operator() (const typeList::struct_decl_error& error) const
	{
		(void) error;
	}

	void operator() (const typeList::new_decl_error& error) const
	{
		(void) error;
	}
};

bool
universe::add_types(const std::string& scope_, const type_decl_list& t)
{
	type_decl_list t_scoped;
	t_scoped.l.resize(t.l.size());
	type_add_scope type_scope(scope_);

	std::transform(t.l.begin(), t.l.end(), 
				   t_scoped.l.begin(), type_scope);

	std::vector<typeList::add_result> res =	tList.add(t_scoped);
	bool isOk = true;
	std::vector<typeList::add_result>::const_iterator it;
	std::vector<type_decl>::const_iterator it1 = t_scoped.l.begin();


	for (it = res.begin(); it != res.end(); ++it, ++it1)
	{
		type_diagnostic_visitor type_diagnostic(*it1);
		bool isSuccess = (*it).get<0>();
		isOk = isOk && isSuccess;
#ifndef NDEBUG
		if (isSuccess) {
			std::cout << "succesfully adding " << boost::apply_visitor(type_decl_name(), *it1);
			std::cout << std::endl;
		} else {
			std::cout << "Failing to add " << boost::apply_visitor(type_decl_name(), *it1);
			std::cout << std::endl;
		}
#endif
		boost::apply_visitor(type_diagnostic, (*it).get<2>());
	}

	return isOk;
}

bool
universe::add_functions(const std::string& scope_, const function_decl_list& f)
{
	function_decl_list f_scoped;
	f_scoped.l.resize(f.l.size());
	functions_def_add_scope add_scope(scope_);

	std::transform(f.l.begin(), f.l.end(),
				   f_scoped.l.begin(), add_scope);

	std::vector<functionDefList::add_result> res = fList.add(f_scoped);

	bool isOk = true;
	std::vector<functionDefList::add_result>::const_iterator it;
	std::vector<function_decl>::const_iterator it1 = f_scoped.l.begin();

	for (it = res.begin(); it != res.end(); ++it, ++it1)
	{
		bool isSuccess = (*it).get<0>();
		isOk = isOk && isSuccess;
		if (isSuccess) {
#ifndef NDEBUG
			std::cout << "Succesfully adding function " << it1->fName << std::endl;
#endif
		} else {
			std::cerr << fList.get_diagnostic(*it1, *it);
		}
	}

	return isOk;
}

bool 
universe::add_rules(const std::string& scope, const rule_decl_list& r)
{
	rule_decl_list r_scoped;
	r_scoped.l.resize(r.l.size());
	rule_add_scope add_scope(scope);

	std::transform(r.l.begin(), r.l.end(),
				   r_scoped.l.begin(), add_scope);

	/* TODO check the consistency of the rules according to the system */
	rList.l.insert(rList.l.end(), r_scoped.l.begin(), r_scoped.l.end());

	return true;
}

bool
universe::add_symbols(const std::string& scope_, const symbol_decl_list& d,
					  symbolList& s)
{
	symbol_decl_list d_scoped;
	d_scoped.l.resize(d.l.size());
	sym_add_scope add_scope(scope_);

	std::transform(d.l.begin(), d.l.end(),
				   d_scoped.l.begin(), add_scope);

	std::vector< symbolList::add_result> res = s.add(d_scoped);

	bool isOk = true;
	std::vector<symbolList::add_result>::const_iterator it;
	std::vector<symbol_decl>::const_iterator it1 = d_scoped.l.begin();

	for (it = res.begin(); it != res.end(); ++it, ++it1)
	{
		bool isSuccess = (*it).first;
		isOk = isOk && isSuccess;
		if (isSuccess) {
#ifndef NDEBUG
			std::cout << "Succesfully adding symbol " << it1->name << std::endl;
#endif
		} else {
			std::cerr << s.get_diagnostic(*it1, *it);
		}
	}

	return isOk;
}

struct print_symbol {
	std::string s1, s2;

	print_symbol(const std::string &set1, const std::string &set2) : 
		s1(set1), s2(set2) {}

	void operator() (const std::string & s)
	{
		std::cerr << "symbol " << s << " is duplicated in ";
		std::cerr << s1 << " variables and ";
		std::cerr << s2 << " variables " << std::endl;
	}
};

bool
universe::add(const ability_decl& decl)
{
	// XXX It is an error ? In case we are using import from different files, it can happens
	// Does parser handle it correctly ?
	abilityMap::const_iterator it = abilities.find(decl.name);
	if (it != abilities.end()) {
		std::cerr << "ability " << decl.name << " already defined  " << std::endl;
		return false;
	}
	
	bool res = add_types(decl.name, decl.blocks.env.types);
	res = add_functions(decl.name, decl.blocks.env.funcs) && res;
	res = add_rules(decl.name, decl.blocks.env.rules) && res;
	symbolList controlables(tList); 
	symbolList readables(tList);
	symbolList privates(tList);
	res = add_symbols(decl.name, decl.blocks.controlables, controlables) && res;
	res = add_symbols(decl.name, decl.blocks.readables, readables) && res;
	res = add_symbols(decl.name, decl.blocks.privates, privates) && res;

	std::vector<std::string> intersect;
	intersect = controlables.intersection(readables);
	if (! intersect.empty()) {
		print_symbol print("controlables", "readables");
		res = false;
		std::for_each(intersect.begin(), intersect.end(), print);
	}

	intersect = controlables.intersection(privates);
	if (! intersect.empty()) {
		print_symbol print("controlables", "privates");
		res = false;
		std::for_each(intersect.begin(), intersect.end(), print);
	}

	intersect = readables.intersection(privates);
	if (! intersect.empty()) {
		print_symbol print("readables", "privates");
		res = false;
		std::for_each(intersect.begin(), intersect.end(), print);
	}

	abilities[decl.name] = 
		boost::make_shared<ability>(decl.name, controlables, readables, privates);

	return res;
}

std::pair<bool, symbolACL>
universe::get_symbol(const std::string& name, const ability& ab,
					 const boost::optional<symbolList>& local_context) const
{
	if (!scope::is_scoped_identifier(name)) {
		std::pair<bool, symbolACL> res = ab.get_symbol(name);
		if (res.first == false && local_context) {
			if (local_context) {
				res = (*local_context).get(name);
				if (res.first == false) {
					std::cerr << "Unknow symbol " << name << " in ability ";
					std::cerr << ab.name() << " nor in local context ";
				}
			} else {
				std::cerr << "Unknow symbol " << name << " in ability " << ab.name();
			}
		}
		return res;
	} else {
		std::pair<std::string, std::string> p;
		p = scope::decompose(name);
		abilityMap::const_iterator it = abilities.find(p.first);
		if (it == abilities.end()) {
			std::cerr << "Ability " << p.first  << " does not exist in expression ";
			std::cerr << name << std::endl;
			return std::make_pair(false, symbolACL());
		}

		std::pair<bool, symbolACL> res;
		res = it->second->get_symbol(p.second);
		if (res.first == false) {
			std::cerr << "Unknow symbol " << name << std::endl;
			return res;
		}

		/*
		 * If we are searching in the current ability (but with a scoped name),
		 * or if it is a remote ability, but a controlable or readable variable
		 * just return the computed symbolACL
		 *
		 * In other case, (remote ability, private variable), return false
		 */
		if (res.first == true && 
				(p.first == ab.name() || res.second.acl != PRIVATE))
			return res;

		std::cerr << "Accessing remote private variable " << name;
		std::cerr << " is forbidden !! " << std::endl;
		return std::make_pair(false, symbolACL());
	}
}

std::pair<bool, functionDef>
universe::get_functionDef(const std::string& name) const
{
	return fList.get(name);
}

struct select_ability_type : public std::unary_function<type, bool>
{
	std::string search_string;

	select_ability_type(const std::string & name) : search_string(name + "::") {};

	bool operator () (const type& t) const
	{
		return (t.name.find(search_string, 0) == 0);
	}
};

struct compute_deps 
{
	std::set<std::string> &s;
	const typeList& tList;

	compute_deps(std::set<std::string>& s_, const typeList& tlist_) :
		s(s_), tList(tlist_) {};

	void operator() (const std::pair<std::string, symbol> & p) const
	{
		type t = tList.get(p.second.t);
		s.insert(scope::get_scope(t.name));
	}
};


struct compute_depends_vis : public boost::static_visitor<void>
{
	std::set<std::string> &s;
	const typeList& tList;

	compute_depends_vis(std::set<std::string> &s_, const typeList& tlist_) : 
		s(s_), tList(tlist_) {};

	void operator() (const Nothing& n) const { (void) n;};

	void operator() (const typeId& t) const
	{
		s.insert(scope::get_scope(tList.get(t).name));
	}

	void operator() (const boost::shared_ptr<symbolList> & l) const
	{
		std::for_each(l->begin(), l->end(), compute_deps(s, tList));
	}
};

struct compute_depends 
{
	std::set<std::string> &s;
	const typeList& tList;

	compute_depends(std::set<std::string>& s_, const typeList& tlist_) :
		s(s_), tList(tlist_) {};

	void operator() (const type& t) const
	{
		boost::apply_visitor(compute_depends_vis(s, tList), t.internal);
	}
};

struct add_include_opaque_type 
{
	std::ostream & oss;
	const typeList& tList;
	const std::string& name;

	add_include_opaque_type(std::ostream& oss, const typeList& tList,
						    const std::string& name):
		oss(oss), tList(tList), name(name)
	{}

	void operator() (const type& t) const
	{
		if (t.t == opaqueType) {
			std::pair<std::string, std::string> p = scope::decompose(t.name);
			oss << "#include <" << name << "/opaque_type_"  << p.second;
			oss << ".hh>\n";
		}
	}
};

std::vector<type> universe::types(const std::string& name) const
{
	return tList.select(select_ability_type(name));

}
bool
universe::define_opaque_type(const std::string& name) const
{
	std::vector<type> a_type = types(name);
	return hyper::utils::any(a_type.begin(), a_type.end(), boost::bind(&type::t, _1) == opaqueType);
}

size_t
universe::dump_ability_types(std::ostream& oss, const std::string& name) const
{
	// find types prefixed by name::
	std::vector<type> a_types = types(name);

	// compute dependances
	std::set<std::string> depends;
	compute_depends deps(depends, tList);
	std::for_each(a_types.begin(), a_types.end(), deps);

	{
		guards g(oss, name, "_TYPE_ABILITY_HH_");

		// include standard needed headers and serialisation header

		oss << "#include <string>\n#include<vector>\n" << std::endl;
		oss << "#include <boost/serialization/serialization.hpp>\n" << std::endl;

		std::for_each(depends.begin(), depends.end(), dump_depends(oss, "types.hh"));

		oss << "\n\n";

		namespaces n(oss, name);
		std::for_each(a_types.begin(), a_types.end(), 
				boost::bind(&type::output, _1, boost::ref(oss), boost::ref(tList)));
	}

	return a_types.size();
}

void
universe::dump_ability_opaque_types(std::ostream& oss, const std::string& name) const
{
	std::vector<type> a_types = types(name);
	{
		guards g(oss, name, "_OPAQUE_TYPE_ABILITY_HH_");

		std::for_each(a_types.begin(), a_types.end(),  add_include_opaque_type(oss, tList, name));
	}
}

struct generated_opaque_include_def
{
	const std::string& directoryName;
	const std::string& name;

	generated_opaque_include_def(const std::string& directoryName, const std::string& name) :
		directoryName(directoryName), name(name) 
	{}

	void operator() (const type& t) const
	{
		if (t.t == opaqueType) {
			std::pair<std::string, std::string> p = scope::decompose(t.name);
			std::ofstream oss((directoryName + "/opaque_type_" + p.second + ".hh").c_str());

			guards g(oss, name, "OPAQUE_TYPE_" + p.second + "_HH_");
			namespaces n(oss, name);
			oss << "\t\t\tstruct " << p.second << "\n";
			oss << "\t\t\t{\n";
			oss << "\t\t\t};\n";
		}
	}
};


void
universe::dump_ability_opaque_types_def(const std::string& directoryName, 
										const std::string& name) const
{
	std::vector<type> a_types = types(name);

	std::for_each(a_types.begin(), a_types.end(), generated_opaque_include_def(directoryName, name));
}

struct select_ability_funs
{
	std::string search_string;

	select_ability_funs(const std::string & name) : search_string(name + "::") {};

	bool operator () (const functionDef& f) const
	{
		return (f.name().find(search_string, 0) == 0);
	}
};

struct compute_fun_depends
{
	std::set<std::string> &depends;
	const typeList& tList;

	compute_fun_depends(std::set<std::string>& depends_, const typeList& tList_) :
		depends(depends_), tList(tList_) {};

	void operator() (const functionDef& f) 
	{
		type t = tList.get(f.returnType());
		depends.insert(scope::get_scope(t.name));

		for (size_t i = 0; i < f.arity(); ++i) 
		{
			type t1 = tList.get(f.argsType(i));
			depends.insert(scope::get_scope(t1.name));
		}
	}
};

struct output_proto_helper 
{
	std::ostream& oss;
	const universe& u;

	output_proto_helper(std::ostream& oss, const universe& u):
		oss(oss), u(u)
	{}

	void operator() (const functionDef& def) const
	{
		boost::optional<std::string> tag = def.tag();
		if (!tag)
			def.output_proto(oss, u.types());
		else
			u.get_extension(*tag).function_proto(oss, def, u.types());
	}
};

struct has_tag {
	bool operator() (const functionDef& def)
	{
		return def.tag();
	}
};

size_t
universe::dump_ability_functions_proto(std::ostream& oss, const std::string& name) const
{
	//find functions prefixed by name::
	std::vector<functionDef>  funcs = fList.select(select_ability_funs(name));
	funcs.erase(std::remove_if(funcs.begin(), funcs.end(), has_tag()), funcs.end());

	// compute dependances
	std::set<std::string> depends;
	compute_fun_depends deps(depends, tList);
	std::for_each(funcs.begin(), funcs.end(), deps);

	{
		guards g(oss, name, "_FUNC_ABILITY_HH_");


		std::for_each(depends.begin(), depends.end(), dump_depends(oss, "types.hh"));

		oss << "#include <string>\n";
		oss << "#include <boost/mpl/vector.hpp>\n\n"; 

		namespaces n(oss, name);
		std::for_each(funcs.begin(), funcs.end(), output_proto_helper(oss, *this));
	}

	return funcs.size();
}

struct output_function_def_impl {
	std::string directoryName;
	std::string name;
	const universe& u;

	output_function_def_impl(const std::string& directoryName, const std::string& name,
							 const universe& u) : 
		directoryName(directoryName), name(name), u(u) {}

	void operator() (const functionDef& def)
	{
		std::string path = directoryName + "/" + scope::get_identifier(def.name()) + ".cc";
		std::ofstream oss(path.c_str());
		const boost::optional<std::string>& tag = def.tag();
		if (tag)
			oss << "#include <" << name << "/" << *tag << "_funcs.hh>\n\n";
		else
			oss << "#include <" << name << "/funcs.hh>\n\n";
		namespaces n(oss, name);
		if (!tag) 
			def.output_impl(oss, u.types());
		else
			u.get_extension(*tag).function_impl(oss, def, u.types());
	}
};

size_t
universe::dump_ability_functions_impl(const std::string& directoryName, const std::string& name) const
{
	//find functions prefixed by name::
	std::vector<functionDef>  funcs = fList.select(select_ability_funs(name));

	std::for_each(funcs.begin(), funcs.end(), 
			output_function_def_impl(directoryName, name, *this));

	return funcs.size();
}


struct truth { bool operator() (const functionDef& fun) { return true; }};

struct extract_tags {
	std::set<std::string>& tag_list;

	extract_tags(std::set<std::string>& tag_list) : tag_list(tag_list) {}

	void operator() (const functionDef& fun)
	{
		if (fun.tag()) 
			tag_list.insert(*fun.tag());
	}
};

struct match_tag {
	std::string tag;

	match_tag(const std::string& tag) : tag(tag) {}

	bool operator() (const functionDef& fun)
	{
		return (fun.tag() && (*(fun.tag()) == tag));
	}
};

size_t 
universe::dump_ability_tagged_functions_proto(const std::string& directoryName, const std::string& name) const
{
	std::vector<functionDef> all_funcs = fList.select(truth());
	std::set<std::string> tag_list;
	std::for_each(all_funcs.begin(), all_funcs.end(), extract_tags(tag_list));

	size_t size = 0;
	std::set<std::string>::const_iterator it;
	for (it = tag_list.begin(); it != tag_list.end(); ++it) {

		const std::string& tag = *it;
		std::string pathName = directoryName + "/" + tag + "_funcs.hh";
		std::ofstream oss(pathName.c_str());
		guards g(oss, name,  "_" + upper(tag) + "_FUNC_ABILITY_HH_");

		std::vector<functionDef> funcs = fList.select(match_tag(tag));
		size+= funcs.size();

		// compute dependances
		std::set<std::string> depends;
		compute_fun_depends deps(depends, tList);
		std::for_each(funcs.begin(), funcs.end(), deps);

		std::for_each(depends.begin(), depends.end(), dump_depends(oss, "types.hh"));

		namespaces n(oss, name);
		std::for_each(funcs.begin(), funcs.end(), output_proto_helper(oss, *this));
	}
	return size;
}

void
universe::dump_ability_import_module_def(std::ostream& oss, const std::string& name) const
{
	oss << "#include <" << name << "/types.hh>" << std::endl;
	oss << "#include <" << name << "/funcs.hh>" << std::endl << std::endl;
	oss << "#include <model/ability.hh>" << std::endl << std::endl;

	namespaces n(oss, name);
	oss << "\t\t\tvoid import_funcs(model::ability &a); " << std::endl;
}

struct output_import_helper {
	std::ostream& oss;
	const universe& u;

	output_import_helper(std::ostream& oss, const universe& u) :
		oss(oss), u(u)
	{}

	void operator() (const functionDef& def) {
		const boost::optional<std::string>& tag = def.tag();
		if (tag) {
			u.get_extension(*tag).output_import(oss, def, u.types());
		} else {
			def.output_import(oss, u.types());
		}
	}
};

struct output_logic_type {
	std::ostream& oss;

	output_logic_type(std::ostream& oss) : oss(oss) {}

	void operator() (const type& t) {
		oss << "\t\t\t\ta.logic().add_equalable_type<" << t.type_name();
		oss << ">(" << quoted_string(t.type_name()) << ");\n";
	}
};

struct is_local_rules {
	std::string scope;

	is_local_rules(const std::string& scope): scope(scope) {}

	bool operator() (const rule_decl& decl) {
		return (scope::get_scope(decl.name) == scope);
	}
};

struct dump_rule {
	std::ostream& oss;
	const ability& a;
	const universe& u;

	dump_rule(std::ostream& oss,
			  const ability &a,
			  const universe& u) : 
		oss(oss), a(a), u(u) {}

	void operator() (const expression_ast& ast) 
	{
		oss << "(" << generate_logic_expression(ast, a, u, true) << ")";
	}
};

struct output_logic_rules {
	std::ostream& oss;
	const ability& a;
	const universe& u;

	output_logic_rules(std::ostream& oss,
					   const ability &a,
					   const universe& u) : 
		oss(oss), a(a), u(u) {}

	void operator() (const rule_decl& r) {
		oss << "\t\t\t\ta.logic().add_rules(" << quoted_string(r.name) << ",\n";
		if (r.premises.empty()) 
			oss << "\t\t\t\t\tstd::vector<std::string>(),\n";
		else {
			oss << "\t\t\t\t\tboost::assign::list_of";
			std::for_each(r.premises.begin(), r.premises.end(), dump_rule(oss, a, u));
			oss << ",\n";
		};
		if (r.conclusions.empty()) 
			oss << "\t\t\t\t\tstd::vector<std::string>());\n";
		else {
			oss << "\t\t\t\t\tboost::assign::list_of";
			std::for_each(r.conclusions.begin(), r.conclusions.end(), dump_rule(oss, a, u));
			oss << ");\n";
		};
	}
};

struct import_types {
	std::string name;

	import_types(const std::string& name) : name(name) {}

	bool operator() (const type& t) {
		std::string scope = scope::get_scope(t.name);

		return (t.is_user_defined() && scope == name);
	}
};

void
universe::dump_ability_import_module_impl(std::ostream& oss, const std::string& name) const
{
	abilityMap::const_iterator it = abilities.find(name);
	if (it == abilities.end()) {
		std::cerr << "ability " << name << " seems to not be defined ! " << std::endl;
		return;
	}

	oss << "#include <" << name << "/import.hh>\n\n";
	oss << "#include <model/logic_layer_impl.hh>\n\n";
	oss << "#include <boost/assign/list_of.hpp>\n\n";

	//find functions prefixed by name::
	std::vector<functionDef>  funcs = fList.select(select_ability_funs(name));

	namespaces n(oss, name);
	oss << "\t\t\tvoid import_funcs(model::ability &a) {" << std::endl;
	std::vector<type> types = tList.select(import_types(name));

	std::for_each(types.begin(), types.end(), output_logic_type(oss));
	std::for_each(funcs.begin(), funcs.end(), output_import_helper(oss, *this));

	std::vector<rule_decl> rules; 
	hyper::utils::copy_if(rList.l.begin(), rList.l.end(), std::back_inserter(rules), 
						  is_local_rules(name));

	std::for_each(rules.begin(), rules.end(), output_logic_rules(oss, *it->second, *this));

	oss << "\t\t\t}" << std::endl;
}

void
universe::dump_ability(std::ostream& oss, const std::string& name) const
{
	abilityMap::const_iterator it = abilities.find(name);
	if (it == abilities.end()) {
		std::cerr << "ability " << name << " seems to not be defined ! " << std::endl;
		return;
	}

	it->second->dump(oss, *this);
}

ability&
universe::get_ability(const std::string& name) 
{
	abilityMap::iterator it = abilities.find(name);
	if (it == abilities.end()) {
		std::cerr << "universe::get_ability : can't retrieve " << name << std::endl;
		assert(false);
	}
	return *(it->second);
}

const ability&
universe::get_ability(const std::string& name) const
{
	abilityMap::const_iterator it = abilities.find(name);
	if (it == abilities.end()) {
		assert(false);
	}
	return *(it->second);
}

void
universe::add_extension(const std::string& name, extension* ext)
{
	extensions[name] = boost::shared_ptr<extension>(ext);
}

const extension&
universe::get_extension(const std::string& name) const
{
	extensionMap::const_iterator it = extensions.find(name);
	assert(it != extensions.end());
	return *(it->second);
}
