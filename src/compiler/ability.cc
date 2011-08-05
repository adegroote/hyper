#include <set>
#include <sstream>
#include <stdexcept>

#include <compiler/ability.hh>
#include <compiler/depends.hh>
#include <compiler/output.hh>
#include <compiler/scope.hh>
#include <compiler/types.hh>
#include <compiler/universe.hh>

#include <boost/bind.hpp>

using namespace hyper::compiler;

std::pair<bool, symbolACL>
ability::get_symbol(const std::string& name) const
{
	std::pair<bool, symbol> p;
	p = controlable_list.get(name);
	if (p.first == true) 
		return std::make_pair(true, symbolACL(p.second, CONTROLABLE));

	p = readable_list.get(name);
	if (p.first == true) 
		return std::make_pair(true, symbolACL(p.second, READABLE));

	p = private_list.get(name);
	if (p.first == true)
		return std::make_pair(true, symbolACL(p.second, PRIVATE));

	return std::make_pair(false, symbolACL());
}

struct same_name {
	std::string name;

	same_name(const std::string& name_): name(name_) {};

	bool operator() (const task& t) const
	{
		return (t.get_name() == name);
	}
};

bool
ability::add_task(const task& t)
{
	/*
	 * Verify that there is no task with the same name
	 * It is not really efficient, but :
	 *   - we don't care a lot about performance here
	 *   - we probably won't have a lot of task, so O(n) against O(ln n) is
	 *   probably not so important
	 */
	std::list<task>::const_iterator it;
	it = std::find_if(tasks.begin(), tasks.end(), same_name(t.get_name()));

	if (it != tasks.end()) {
		std::cerr << "Can't add the task named " << t.get_name();
		std::cerr << " because a task with the same already exists" << std::endl;
		return false;
	}

	tasks.push_back(t);
	return true;
}

const task& 
ability::get_task(const std::string& name) const
{
	std::list<task>::const_iterator it;
	it = std::find_if(tasks.begin(), tasks.end(), same_name(name));
	if (it == tasks.end())
		throw std::runtime_error("Unknow task " + name);
	return *it;
}

task& 
ability::get_task(const std::string& name) 
{
	std::list<task>::iterator it;
	it = std::find_if(tasks.begin(), tasks.end(), same_name(name));
	if (it == tasks.end())
		throw std::runtime_error("Unknow task " + name);
	return *it;
}


struct compute_type_depends 
{
	std::set<std::string> &s;
	const typeList& tList;

	compute_type_depends(std::set<std::string>& s_, const typeList& tlist_) :
		s(s_), tList(tlist_) {};

	void operator() (const std::pair<std::string, symbol>& p) const
	{
		type t = tList.get(p.second.t);
		s.insert(scope::get_scope(t.name));
	}
};

struct compute_fun_depends
{
	depends& d;
       const std::string& name;
       const universe& u;

       compute_fun_depends(depends& d_, const std::string& n, const universe& u) :
               d(d_), name(n), u(u) {};

       void operator() (const task& t) const
       {
               void (*f)(const expression_ast&, const std::string&, const universe&,  depends&)
                      = &add_depends;
               std::for_each(t.pre_begin(), t.pre_end(), 
                               boost::bind(f, _1, boost::cref(name),
                                                  boost::cref(u),
                                                  boost::ref(d)));
               std::for_each(t.post_begin(), t.post_end(), 
                               boost::bind(f, _1, boost::cref(name),
                                                  boost::cref(u),
                                                  boost::ref(d)));
       }
};

struct print_initializer_helpers : boost::static_visitor<std::string>
{
	std::string var_name;

	print_initializer_helpers(const std::string& name) : var_name(name) {}

	template <typename T>
	std::string operator () (const T&) const { return ""; }

	std::string operator () (const expression_ast& e) const
	{
		return boost::apply_visitor(print_initializer_helpers(var_name), e.expr);
	}

	template <typename T>
	std::string operator () (const Constant<T>& c) const
	{
		std::ostringstream oss;
		oss << ", " << var_name << "(" << c.value << ")";
		return oss.str();
	}

	std::string operator () (const Constant<std::string>& c) const
	{
		std::ostringstream oss;
		oss << ", " << var_name << "(" << quoted_string(c.value) << ")";
		return oss.str();
	}
};

std::string print_initializer(const expression_ast& init, const std::string& name)
{
	return boost::apply_visitor(print_initializer_helpers(name), init.expr);
}

struct print_symbol
{
	std::ostream& oss;
	const typeList& tList;
	const std::string& name;
	
	print_symbol(std::ostream& oss_, const typeList& tList_, const std::string& name_) :
		oss(oss_), tList(tList_), name(name_) {};

	void operator () (const std::pair<std::string, symbol>& p) const
	{
		type t = tList.get(p.second.t);
		oss << "\t\t\t\t" << t.type_name(); //scope::get_context_identifier(t.name, name);
		oss << " " << p.second.name << ";" << std::endl;
	}
};


struct initialize_variable 
{
	std::ostream& oss;

	initialize_variable(std::ostream& oss_) : oss(oss_) {};

	void operator () (const std::pair<std::string, symbol>& p) const
	{
		oss << print_initializer(p.second.initializer, p.second.name);
	}
};

struct compute_updater : public boost::static_visitor<std::string>
{
	template <typename T>
	std::string operator () (const T&) const { return ""; }

	std::string operator () (const expression_ast& e) const
	{
		return boost::apply_visitor(compute_updater(), e.expr);
	}

	std::string operator () (const std::string& s) const
	{
		return quoted_string(s);
	}

	std::string operator () (const function_call& f) const
	{
		return quoted_string(f.fName);
	}
};

struct add_proxy_symbol
{
	std::ostream& oss;

	add_proxy_symbol(std::ostream& oss_) : oss(oss_) {};

	void operator () (const std::pair<std::string, symbol>& p) const
	{
		std::string updater = 
			boost::apply_visitor(compute_updater(), p.second.initializer.expr);
		oss << "\t\t\t\t\texport_variable(" << quoted_string(p.second.name) << ", ";
		oss << p.second.name;
		if (updater == "") 
			oss << ");" << std::endl;
		else 
			oss << ", " << updater << ");" << std::endl;
	}
};

struct add_setter_symbol
{
	std::ostream& oss;

	add_setter_symbol(std::ostream& oss_) : oss(oss_) {};

	void operator() (const std::pair<std::string, symbol>& p) const
	{
		oss << "\t\t\t\t\texport_writable_variable(" << quoted_string(p.second.name) << ", ";
		oss << p.second.name << ");\n";
	}
};

struct add_import_funcs
{
	std::ostream& oss;

	add_import_funcs(std::ostream& oss_) : oss(oss_) {};

	void operator () (const std::string& s) const
	{
		if (s != "")
			oss << "\t\t\t\t\t" << s << "::import_funcs(*this);" << std::endl;
	}
};

depends 
ability::get_function_depends(const universe& u) const
{
       depends d;
       compute_fun_depends fun_deps(d, name_, u);
       std::for_each(tasks.begin(), tasks.end(), fun_deps);
       return d;
}

struct add_task_header
{
	std::ostream &oss;
	std::string abilityName;

	add_task_header(std::ostream& oss_, const std::string& name) : 
		oss(oss_), abilityName(name) {};

	void operator() (const task& t) const
	{
		oss << "#include <" << abilityName << "/tasks/" << t.get_name();
		oss << ".hh>" << std::endl;
	}
};

struct add_task_declaration
{
	std::ostream &oss;

	add_task_declaration(std::ostream& oss_) : oss(oss_) {}

	void operator() (const task& t) const
	{
		oss << times(5, "\t");
		oss << "logic().tasks.insert(std::make_pair(";
		oss << quoted_string(t.get_name()) << ", \n";
		oss << times(12, "\t") << "model::task_ptr(new ";
		oss << t.exported_name() << "(*this))));" << std::endl;
	}
};

void 
ability::dump_include(std::ostream& oss, const universe& u) const
{
	std::set<std::string> type_depends;
	compute_type_depends type_deps(type_depends, u.types());
	std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
	std::for_each(readable_list.begin(), readable_list.end(), type_deps);
	std::for_each(private_list.begin(), private_list.end(), type_deps);

	guards g(oss, name_, "_ABILITY_HH_");

	oss << "#include <model/ability.hh>\n";
	std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
	std::for_each(readable_list.begin(), readable_list.end(), type_deps);
	std::for_each(private_list.begin(), private_list.end(), type_deps);
	std::for_each(type_depends.begin(), type_depends.end(), dump_depends(oss, "types.hh"));
	oss << std::endl;

	namespaces n(oss, name_);

	print_symbol print(oss, u.types(), name_);
	oss << "\t\t\tstruct ability : public model::ability {" << std::endl;

	std::for_each(controlable_list.begin(), controlable_list.end(), print);
	std::for_each(readable_list.begin(), readable_list.end(), print);
	std::for_each(private_list.begin(), private_list.end(), print);

	oss << "\t\t\t\tability(int level);\n";
	oss << "\t\t\t};\n";
}

void
ability::dump(std::ostream& oss, const universe& u) const
{
	std::set<std::string> type_depends;
	compute_type_depends type_deps(type_depends, u.types());
	std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
	std::for_each(readable_list.begin(), readable_list.end(), type_deps);
	std::for_each(private_list.begin(), private_list.end(), type_deps);
	depends deps = get_function_depends(u);

	oss << "#include <" << name_ << "/ability.hh>\n";

	std::set<std::string> import_depends(type_depends);
	import_depends.insert(deps.fun_depends.begin(), deps.fun_depends.end());; 
	std::for_each(import_depends.begin(), import_depends.end(), dump_depends(oss, "import.hh"));
	oss << std::endl;
	std::for_each(tasks.begin(), tasks.end(), add_task_header(oss, name_));
	oss << "#include <model/ability_impl.hh>" << std::endl;
	oss << "#include <model/logic_layer_impl.hh>" << std::endl;

	namespaces n(oss, name_);
	oss << "\t\t\t\tability::ability(int level) : model::ability(" << quoted_string(name_) << ", level)";
	initialize_variable initialize(oss);
	std::for_each(controlable_list.begin(), controlable_list.end(), initialize);
	std::for_each(readable_list.begin(), readable_list.end(), initialize);
	std::for_each(private_list.begin(), private_list.end(), initialize);
	oss << "{\n" ;
	std::for_each(controlable_list.begin(), controlable_list.end(), add_proxy_symbol(oss));
	std::for_each(readable_list.begin(), readable_list.end(), add_proxy_symbol(oss));
	std::for_each(controlable_list.begin(), controlable_list.end(), add_setter_symbol(oss));
	std::for_each(import_depends.begin(), import_depends.end(), add_import_funcs(oss));
	std::for_each(tasks.begin(), tasks.end(), add_task_declaration(oss));
	oss << "\t\t\t\t\tstart();\n;";
	oss << "\t\t\t\t}\n";
}

std::set<std::string>
ability::get_type_depends(const typeList& tList) const
{
	std::set<std::string> type_depends;
	compute_type_depends type_deps(type_depends, tList);
	std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
	std::for_each(readable_list.begin(), readable_list.end(), type_deps);
	std::for_each(private_list.begin(), private_list.end(), type_deps);

	return type_depends;
}

struct agent_export_symbol
{
	std::ostream& oss;
	const typeList& tList;
	const std::string& name;
	
	agent_export_symbol(std::ostream& oss_, const typeList& tList_, const std::string& name_) :
		oss(oss_), tList(tList_), name(name_) {};

	void operator () (const std::pair<std::string, symbol>& p) const
	{
		type t = tList.get(p.second.t);
		oss << "\t\t\thyper::model::future_value<";
		oss << t.type_name();
		oss << "> " << p.second.name << "();" << std::endl;
	}
};

void
ability::agent_export_declaration(std::ostream& oss, const typeList& tList) const
{
	std::set<std::string> type_depends;
	compute_type_depends type_deps(type_depends, tList);
	std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
	std::for_each(readable_list.begin(), readable_list.end(), type_deps);

	guards g(oss, name_, "_EXPORT_AGENT_HH_");

	oss << "#include <model/ability_test.hh>\n\n";

	std::for_each(type_depends.begin(), type_depends.end(), dump_depends(oss, "types.hh"));
	oss << std::endl;

	namespaces n(oss, name_);


	agent_export_symbol print(oss, tList, name_);
	
	oss << "\t\tstruct ability_test : public model::ability_test {\n" ;
	oss << "\t\t\tability_test() : model::ability_test(" << quoted_string(name_);
	oss << ") {} \n" << std::endl;
	std::for_each(controlable_list.begin(), controlable_list.end(), print);
	std::for_each(readable_list.begin(), readable_list.end(), print);
	oss << "\t\t\thyper::model::future_value<bool> enforce(const std::string& ctr, bool repeat) {\n";
	oss << "\t\t\t\treturn send_constraint(ctr, repeat);\n";
	oss << "\t\t\t}\n";
	oss << "\t\t\tvoid exit(const std::string& msg) {\n";
	oss << "\t\t\t\treturn abort(msg);\n";
	oss << "\t\t\t}\n";
	oss << "\t\t};" << std::endl;
}

struct agent_export_impl
{
	std::ostream& oss;
	const typeList& tList;
	const std::string& name;
	
	agent_export_impl(std::ostream& oss_, const typeList& tList_, const std::string& name_) :
		oss(oss_), tList(tList_), name(name_) {};

	void operator () (const std::pair<std::string, symbol>& p) const
	{
		type t = tList.get(p.second.t);
		oss << "hyper::model::future_value<" << t.type_name();
		oss << "> ability_test::"  << p.second.name << "()" << std::endl;
		oss << "{" << std::endl;
		oss << "\treturn get_value<";
		oss << t.type_name();
		oss << ">(" << quoted_string(p.second.name) << ");";
		oss << std::endl;
		oss << "}" << std::endl;
	}
};

void 
ability::agent_export_implementation(std::ostream& oss, const typeList& tList) const
{
	oss << "#include <network/rpc.hh>" << std::endl;
	oss << "#include <network/msg.hh>" << std::endl;
	oss << "#include <model/get_value.hh>" << std::endl;
	oss << "#include <" << name_ << "/export.hh>" << std::endl;
	oss << "using namespace hyper;" << std::endl;
	oss << "using namespace hyper::" << name_ << ";" << std::endl;
	oss << std::endl;

	agent_export_impl print(oss, tList, name_);
	std::for_each(controlable_list.begin(), controlable_list.end(), print);
	std::for_each(readable_list.begin(), readable_list.end(), print);
}

struct dump_swig {
	std::ostream& oss;

	dump_swig(std::ostream& oss) : oss(oss) {}

	void operator() (const type& t) const {
		std::string type_name;
		if (scope::is_scoped_identifier(t.name)) {
			std::pair<std::string, std::string> p;
			p = scope::decompose(t.name);
			type_name = p.second;
		} else {
			type_name = t.name;
		}
		oss << "%template(Future_" << type_name << ") ";
		oss << "hyper::model::future_value<";
		if (scope::is_basic_identifier(t.name)) 
			oss << t.type_name();
		else 
			oss << "hyper::" << t.type_name();
		oss << ">;\n";
	}
};

struct extract_types
{
	std::set<type> &s;
	const typeList& tList;

	extract_types(std::set<type>& s_, const typeList& tlist_) :
		s(s_), tList(tlist_) {};

	void operator() (const std::pair<std::string, symbol>& p) const
	{
		s.insert(tList.get(p.second.t));
	}
};

void
ability::dump_swig_ability_types(std::ostream& oss, const typeList& tList) const
{
	{
		// search the list of ability we depends on
		std::set<std::string> type_depends;
		compute_type_depends type_deps(type_depends, tList);
		std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
		std::for_each(readable_list.begin(), readable_list.end(), type_deps);

		std::for_each(type_depends.begin(), type_depends.end(), dump_depends(oss, "types.hh", '%'));
	}

	{
		// search the list of type used in the variable 
		std::set<type> types;
		extract_types type_deps(types, tList);
		std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
		std::for_each(readable_list.begin(), readable_list.end(), type_deps);
	
		std::for_each(types.begin(), types.end(), dump_swig(oss));
	}
}
