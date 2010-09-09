#include <set>

#include <compiler/ability.hh>
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
		return (t.name == name);
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
	std::vector<task>::const_iterator it;
	it = std::find_if(tasks.begin(), tasks.end(), same_name(t.name));

	if (it != tasks.end()) {
		std::cerr << "Can't add the task named " << t.name;
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

struct compute_expression_deps : public boost::static_visitor<void>
{
	std::set<std::string>& s;
	const typeList& tList;
	const std::string& name;

	compute_expression_deps(std::set<std::string>& s_, const typeList& tlist_, 
			const std::string& name_) :
		s(s_), tList(tlist_), name(name_) {};

	template <typename T>
	void operator() (const T& e) const { (void)e; }

	void operator() (const expression_ast& e) const
	{
		boost::apply_visitor(compute_expression_deps(s, tList, name), e.expr);
	}

	void operator() (const function_call& f) const
	{
		std::string scope = scope::get_scope(f.fName);
		if (scope == "") // a function without scope means that the function is part of the local scope
			scope = name;
		s.insert(scope);
		for (size_t i = 0; i < f.args.size(); ++i) 
			boost::apply_visitor(compute_expression_deps(s, tList, name), f.args[i].expr);
	}

	template <binary_op_kind T>
	void  operator() (const binary_op<T> & op) const
	{
		boost::apply_visitor(compute_expression_deps(s, tList, name), op.left.expr);
		boost::apply_visitor(compute_expression_deps(s, tList, name), op.right.expr); 
	}

	template <unary_op_kind T>
	void operator() (const unary_op<T>& op) const
	{
		boost::apply_visitor(compute_expression_deps(s, tList, name), op.subject.expr);
	}
};

struct compute_fun_expression_depends
{
	std::set<std::string>& s;
	const typeList& tList;
	const std::string& name;

	compute_fun_expression_depends(std::set<std::string>& s_, const typeList& tlist_,
			const std::string &name_) :
		s(s_), tList(tlist_), name(name_) {};

	void operator() (const expression_ast& e) const
	{
		boost::apply_visitor(compute_expression_deps(s, tList, name), e.expr);
	}
};


struct compute_fun_depends
{
	std::set<std::string>& s;
	const typeList& tList;
	const std::string& name;

	compute_fun_depends(std::set<std::string>& s_, const typeList& tlist_, const std::string& n) :
		s(s_), tList(tlist_), name(n) {};

	void operator() (const task& t) const
	{
		compute_fun_expression_depends c(s, tList, name);
		std::for_each(t.pre.begin(), t.pre.end(), c);
		std::for_each(t.post.begin(), t.post.end(), c);
	}
};

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

void
ability::dump(std::ostream& oss, const typeList& tList, const universe& u) const
{
	std::set<std::string> type_depends;
	compute_type_depends type_deps(type_depends, tList);
	std::for_each(controlable_list.begin(), controlable_list.end(), type_deps);
	std::for_each(readable_list.begin(), readable_list.end(), type_deps);
	std::for_each(private_list.begin(), private_list.end(), type_deps);

	std::set<std::string> fun_depends;
	compute_fun_depends fun_deps(fun_depends, tList, name_);
	std::for_each(tasks.begin(), tasks.end(), fun_deps);

	guards g(oss, name_, "_ABILITY_HH_");

	std::for_each(type_depends.begin(), type_depends.end(), dump_depends(oss, "types.hh"));
	oss << std::endl;
	std::for_each(fun_depends.begin(), fun_depends.end(), dump_depends(oss, "funcs.hh"));
	oss << std::endl;
	oss << "#include <model/ability.hh>" << std::endl << std::endl;
	oss << "#include <model/task.hh>" << std::endl << std::endl;

	namespaces n(oss, name_);

	print_symbol print(oss, tList, name_);
	oss << "\t\t\tstruct ability : public model::ability {" << std::endl;

	std::for_each(tasks.begin(), tasks.end(),
				  boost::bind(&task::dump, _1, boost::ref(oss), 
											   boost::cref(tList), 
											   boost::cref(u),
											   boost::cref(*this)));

	std::for_each(controlable_list.begin(), controlable_list.end(), print);
	std::for_each(readable_list.begin(), readable_list.end(), print);
	std::for_each(private_list.begin(), private_list.end(), print);

	oss << "\t\t\t\tability() : model::ability(\"" << name_ << "\") {\n" ;
	std::for_each(controlable_list.begin(), controlable_list.end(), add_proxy_symbol(oss));
	std::for_each(readable_list.begin(), readable_list.end(), add_proxy_symbol(oss));
	oss << "\t\t\t\t}\n;" << std::endl;
	oss << "\t\t\t};" << std::endl;
}
