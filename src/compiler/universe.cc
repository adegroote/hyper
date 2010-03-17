#include <boost/bind.hpp>

#include <compiler/universe.hh>

using namespace hyper::compiler;

universe::universe() : tList(), fList(tList)
{
	// Add basic native types
	
	tList.add("bool", boolType);
	tList.add("double", doubleType);
	tList.add("int", intType);
	tList.add("string", stringType);
	tList.add("void", noType);

	basicIdentifier.push_back("bool");
	basicIdentifier.push_back("double");
	basicIdentifier.push_back("int");
	basicIdentifier.push_back("string");
	basicIdentifier.push_back("void");
}

bool
universe::is_scoped_identifier(const std::string& s) const
{
	std::string::size_type res = s.find("::");
	return (res != std::string::npos);
}

bool
universe::is_basic_identifier(const std::string& s) const
{
	// XXX basicIdentifier is not sorted ?
	std::vector<std::string>::const_iterator begin, end;
	begin = basicIdentifier.begin();
	end = basicIdentifier.end();

	return (std::find(begin, end, s) != end);
}

std::string
universe::add_scope(const std::string& abilityName, const std::string& id) const
{
	if (is_scoped_identifier(id) || is_basic_identifier(id))
		return id;
	
	return abilityName + "::" + id;
}

struct sym_add_scope 
{
	std::string scope;
	const universe& u;

	sym_add_scope(const std::string& s, const universe& u_) : scope(s), u(u_) {};
	symbol_decl operator() (const symbol_decl &s)
	{
		symbol_decl res = s;
		res.typeName = u.add_scope(scope, res.typeName);
		return res;
	}
};

struct type_add_scope_visitor : public boost::static_visitor<type_decl>
{
	std::string scope;
	const universe& u;
	type_add_scope_visitor(const std::string& s, const universe& u_) : 
		scope(s), u(u_) {};

	type_decl operator()(const newtype_decl& decl)
	{
		newtype_decl res = decl;
		res.oldname = u.add_scope(scope, res.oldname);
		res.newname = u.add_scope(scope, res.newname);
		return res;
	}

	type_decl operator()(const struct_decl& decl)
	{
		struct_decl res;
		sym_add_scope sym_scope(scope, u);
		res.name = u.add_scope(scope, decl.name);
		res.vars.l.resize(decl.vars.l.size());
		std::transform(decl.vars.l.begin(), decl.vars.l.end(),
						res.vars.l.begin(), sym_scope);
		return res;
	}
};

struct type_add_scope
{
	std::string scope;
	const universe& u;

	type_add_scope(const std::string& s, const universe& u_) : scope(s), u(u_) {};

	type_decl operator()(const type_decl& decl)
	{
		type_add_scope_visitor v(scope, u);
		return boost::apply_visitor(v, decl);
	}
};

struct functions_def_add_scope {
	std::string scope;
	const universe& u;

	functions_def_add_scope(const std::string& s, const universe& u_) : scope(s), u(u_) {};

	function_decl operator()(const function_decl& decl)
	{
		function_decl res = decl;
		res.fName = u.add_scope(scope, res.fName);
		res.returnName = u.add_scope(scope, res.returnName);
		
		std::transform(res.argsName.begin(), res.argsName.end(),
					   res.argsName.begin(), boost::bind(&universe::add_scope, &u, scope, _1));
		return res;
	}
};

struct type_diagnostic_visitor : public boost::static_visitor<void>
{
	const type_decl &decl;

	type_diagnostic_visitor(const type_decl& decl_) : decl(decl_) {};

	void operator() (const typeList::native_decl_error& error) const
	{
		assert(false);
	}

	void operator() (const typeList::struct_decl_error& error) const
	{
	}

	void operator() (const typeList::new_decl_error& error) const
	{
	}
};

bool
universe::add_types(const std::string& scope, const type_decl_list& t)
{
	type_decl_list t_scoped;
	t_scoped.l.resize(t.l.size());
	type_add_scope type_scope(scope, *this);

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
		if (isSuccess) {
			std::cout << "succesfully adding " << boost::apply_visitor(type_decl_name(), *it1);
			std::cout << std::endl;
		} else {
			std::cout << "Failing to add " << boost::apply_visitor(type_decl_name(), *it1);
			std::cout << std::endl;
		}
		boost::apply_visitor(type_diagnostic, (*it).get<2>());
	}

	return isOk;
}

bool
universe::add_functions(const std::string& scope, const function_decl_list& f)
{
	function_decl_list f_scoped;
	f_scoped.l.resize(f.l.size());
	functions_def_add_scope add_scope(scope, *this);

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
			std::cout << "Succesfully adding function " << it1->fName << std::endl;
		} else {
			std::cerr << fList.get_diagnostic(*it1, *it);
		}
	}

	return isOk;
}


bool
universe::add(const ability_decl& decl)
{
	bool res = add_types(decl.name, decl.blocks.env.types);
	res = res && add_functions(decl.name, decl.blocks.env.funcs);
	return res;
}
