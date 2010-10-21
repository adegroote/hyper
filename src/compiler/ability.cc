#include <set>
#include <sstream>

#include <compiler/ability.hh>
#include <compiler/depends.hh>
#include <compiler/output.hh>
#include <compiler/scope.hh>
#include <compiler/types.hh>

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
	std::set<std::string>& s;
	const std::string& name;

	compute_fun_depends(std::set<std::string>& s_, const std::string& n) :
		s(s_), name(n) {};

	void operator() (const task& t) const
	{
		std::for_each(t.pre_begin(), t.pre_end(), 
				boost::bind(&add_depends, _1, boost::cref(name),
											  boost::ref(s)));
		std::for_each(t.post_begin(), t.post_end(), 
				boost::bind(&add_depends, _1, boost::cref(name),
											  boost::ref(s)));
	}
};

struct print_initializer_helpers : boost::static_visitor<std::string>
{
	std::string var_name;

	print_initializer_helpers(const std::string& name) : var_name(name) {}

	template <typename T>
	std::string operator () (const T&) const { assert(false); return ""; }

	std::string operator () (const empty &) const { return ""; }

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

	std::string operator () (const function_call& f) const
	{
		assert(f.args.size() == 0);
		// XXX TODO, implement 
		return "";
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
		oss << "\t\t\t\t" << scope::get_context_identifier(t.name, name);
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

struct add_proxy_symbol
{
	std::ostream& oss;

	add_proxy_symbol(std::ostream& oss_) : oss(oss_) {};

	void operator () (const std::pair<std::string, symbol>& p) const
	{
		oss << "\t\t\t\t\texport_variable(\"" << p.second.name << "\",";
		oss << p.second.name << ");" << std::endl;
	}
};

struct add_import_funcs
{
	std::ostream& oss;

	add_import_funcs(std::ostream& oss_) : oss(oss_) {};

	void operator () (const std::string& s) const
	{
		oss << "\t\t\t\t\t" << s << "::import_funcs(*this);" << std::endl;
	}
};



std::set<std::string> 
ability::get_function_depends() const
{
	std::set<std::string> fun_depends;
	compute_fun_depends fun_deps(fun_depends, name_);
	std::for_each(tasks.begin(), tasks.end(), fun_deps);
	return fun_depends;
}

void
ability::dump(std::ostream& oss, const typeList& tList, const universe& u) const
{
	std::set<std::string> type_depends;
	compute_type_depends type_deps(type_depends, tList);
	std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
	std::for_each(readable_list.begin(), readable_list.end(), type_deps);
	std::for_each(private_list.begin(), private_list.end(), type_deps);

	std::set<std::string> fun_depends = get_function_depends();

	guards g(oss, name_, "_ABILITY_HH_");

	std::for_each(type_depends.begin(), type_depends.end(), dump_depends(oss, "types.hh"));
	oss << std::endl;
	std::for_each(fun_depends.begin(), fun_depends.end(), dump_depends(oss, "import.hh"));
	oss << std::endl;
	oss << "#include <model/ability.hh>" << std::endl;

	namespaces n(oss, name_);

	print_symbol print(oss, tList, name_);
	oss << "\t\t\tstruct ability : public model::ability {" << std::endl;

	std::for_each(controlable_list.begin(), controlable_list.end(), print);
	std::for_each(readable_list.begin(), readable_list.end(), print);
	std::for_each(private_list.begin(), private_list.end(), print);

	initialize_variable initialize(oss);
	oss << "\t\t\t\tability() : model::ability(\"" << name_ << "\")";
	std::for_each(controlable_list.begin(), controlable_list.end(), initialize);
	std::for_each(readable_list.begin(), readable_list.end(), initialize);
	std::for_each(private_list.begin(), private_list.end(), initialize);
	oss << "{\n" ;
	std::for_each(controlable_list.begin(), controlable_list.end(), add_proxy_symbol(oss));
	std::for_each(readable_list.begin(), readable_list.end(), add_proxy_symbol(oss));
	std::for_each(fun_depends.begin(), fun_depends.end(), add_import_funcs(oss));
	oss << "\t\t\t\t}\n;" << std::endl;
	oss << "\t\t\t};" << std::endl;
}
