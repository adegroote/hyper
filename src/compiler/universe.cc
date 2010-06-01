#include <set>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

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

std::pair<std::string, std::string>
universe::decompose(const std::string& name) const
{
	std::string::size_type res = name.find("::");
	assert (res != std::string::npos);
	return std::make_pair(
			name.substr(0, res),
			name.substr(res+2, name.size() - (res+2)));
}

std::string
universe::get_scope(const std::string& name) const
{
	if (!is_scoped_identifier(name)) return "";
	std::pair<std::string, std::string> p = decompose(name);
	return p.first;
}

std::string 
universe::get_identifier(const std::string& identifier) const
{
	if (!is_scoped_identifier(identifier))
		return identifier;
	std::pair<std::string, std::string> p = decompose(identifier);
	return p.second;
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
universe::add_symbols(const std::string& scope, const symbol_decl_list& d,
					  symbolList& s)
{
	symbol_decl_list d_scoped;
	d_scoped.l.resize(d.l.size());
	sym_add_scope add_scope(scope, *this);

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
			std::cout << "Succesfully adding symbol " << it1->name << std::endl;
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
universe::get_symbol(const std::string& name, const boost::shared_ptr<ability>& pAbility) const
{
	if (!is_scoped_identifier(name)) {
		std::pair<bool, symbolACL> res = pAbility->get_symbol(name);
		if (res.first == false)
			std::cerr << "Unknow symbol " << name << " in ability " << pAbility->name() << std::endl;
		return res;
	} else {
		std::pair<std::string, std::string> p;
		p = decompose(name);
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
				(p.first == pAbility->name() || res.second.acl != PRIVATE))
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

/*
 * Classify the binary_op into two kind
 *   - logical, returns a bool
 *   - numerical, the return type depends on the left operand (left and right
 *   operand must be the same)
 *   XXX : use "typeclass" to check that the operation has "sense"
 */

enum kind_of_op { NONE, NUMERICAL, LOGICAL};
template <binary_op_kind T> struct TypeOp { enum { value = NONE }; };
template <> struct TypeOp<ADD> { enum { value = NUMERICAL }; };
template <> struct TypeOp<SUB> { enum { value = NUMERICAL }; };
template <> struct TypeOp<MUL> { enum { value = NUMERICAL }; };
template <> struct TypeOp<DIV> { enum { value = NUMERICAL }; };
template <> struct TypeOp<AND> { enum { value = LOGICAL }; };
template <> struct TypeOp<OR> { enum { value = LOGICAL }; };
template <> struct TypeOp<GT> { enum { value = LOGICAL }; };
template <> struct TypeOp<GTE> { enum { value = LOGICAL }; };
template <> struct TypeOp<LT> { enum { value = LOGICAL }; };
template <> struct TypeOp<LTE> { enum { value = LOGICAL }; };
template <> struct TypeOp<EQ> { enum { value = LOGICAL }; };
template <> struct TypeOp<NEQ> { enum { value = LOGICAL }; };

template <binary_op_kind T, int k>
struct binary_type {
	const universe& u;
	typeId id;	
	
	binary_type(const universe& u_, typeId id_) : u(u_), id(id_) {};

	typeId operator() () const { return -1; };
};

template <binary_op_kind T>
struct binary_type<T, NUMERICAL> {
	const universe& u;
	typeId id;	
	
	binary_type(const universe& u_, typeId id_) : u(u_), id(id_) {};

	typeId operator() () const { return id; };
};

template <binary_op_kind T>
struct binary_type<T, LOGICAL> {
	const universe& u;
	typeId id;	
	
	binary_type(const universe& u_, typeId id_) : u(u_), id(id_) {};

	typeId operator() () const { return u.types().getId("bool").second; };
};

/*
 * Compute the type of an expression
 * We assume that the expression is valid
 */
struct ast_type : public boost::static_visitor<typeId> {
	boost::shared_ptr<ability> pAbility;
	const universe& u;

	ast_type(boost::shared_ptr<ability> pAbility_, const universe& u_):
		pAbility(pAbility_), u(u_) 
	{}

	typeId operator() (const empty& e) const
	{
		return -1;
	}

	typeId operator() (const Constant<int>& c) const
	{
		return u.types().getId("int").second;
	}

	typeId operator() (const Constant<double>& c) const
	{
		return u.types().getId("double").second;
	}

	typeId operator() (const Constant<std::string>& c) const
	{
		return u.types().getId("string").second;
	}

	typeId operator() (const Constant<bool>& c) const
	{
		return u.types().getId("bool").second;
	}

	typeId operator() (const std::string& s) const
	{
		std::pair<bool, symbolACL> p;
		p = u.get_symbol(s, pAbility);
		if (p.first == false)
			return -1;
		return p.second.s.t; 
	}

	typeId operator() (const function_call& f) const
	{
		// add scope to do the search
		std::string name = u.add_scope(pAbility->name(), f.fName);
		std::pair<bool, functionDef> p = u.get_functionDef(name);
		if (p.first == false) 
			return -1;

		return p.second.returnType();
	}

	bool operator() (const expression_ast& e) const
	{
		return boost::apply_visitor(ast_type(pAbility, u), e.expr);
	}

	template<binary_op_kind T>
	bool operator() (const binary_op<T>& b) const
	{
		typeId leftId = boost::apply_visitor(ast_type(pAbility, u), b.left.expr);
		return binary_type<T, TypeOp<T>::value> (u, leftId) ();
	}

	bool operator() (const unary_op<NEG>& op) const
	{
		return u.types().getId("bool").second;
	}
};
	

typeId
universe::typeOf(const boost::shared_ptr<ability>& pAbility, const expression_ast& expr) const
{
	return boost::apply_visitor(ast_type(pAbility, *this), expr.expr);
}

template <unary_op_kind T>
struct ast_unary_valid 
{
	const universe& u;

	ast_unary_valid(const universe& u_) : u(u_) {};

	bool operator() (typeId id) const
	{
		return false;
	}
};

template <>
struct ast_unary_valid<NEG>
{
	const universe& u;

	ast_unary_valid(const universe& u_) : u(u_) {};

	bool operator() (typeId id) const
	{
		return (id == u.types().getId("int").second || 
				id == u.types().getId("double").second);
	}
};

enum valid_kind_of { VALID_NONE, VALID_NUMERICAL, VALID_COMPARABLE, VALID_LOGICAL };
template <binary_op_kind T> struct ValidTypeOp { enum { value = VALID_NONE }; };
template <> struct ValidTypeOp<ADD> { enum { value = VALID_NUMERICAL }; };
template <> struct ValidTypeOp<SUB> { enum { value = VALID_NUMERICAL }; };
template <> struct ValidTypeOp<MUL> { enum { value = VALID_NUMERICAL }; };
template <> struct ValidTypeOp<DIV> { enum { value = VALID_NUMERICAL }; };
template <> struct ValidTypeOp<AND> { enum { value = VALID_LOGICAL }; };
template <> struct ValidTypeOp<OR> { enum { value = VALID_LOGICAL }; };
template <> struct ValidTypeOp<GT> { enum { value = VALID_COMPARABLE }; };
template <> struct ValidTypeOp<GTE> { enum { value = VALID_COMPARABLE}; };
template <> struct ValidTypeOp<LT> { enum { value = VALID_COMPARABLE}; };
template <> struct ValidTypeOp<LTE> { enum { value = VALID_COMPARABLE}; };
template <> struct ValidTypeOp<EQ> { enum { value = VALID_COMPARABLE}; };
template <> struct ValidTypeOp<NEQ> { enum { value = VALID_COMPARABLE}; };

template <binary_op_kind T, int k>
struct ast_binary_valid_dispatch
{
	const universe& u;

	ast_binary_valid_dispatch(const universe& u_) : u(u_) {};

	bool operator() (typeId leftType, typeId rightType)
	{
		return false;
	}
};

template <binary_op_kind T>
struct ast_binary_valid_dispatch<T, VALID_NUMERICAL>
{
	const universe& u;

	ast_binary_valid_dispatch(const universe& u_) : u(u_) {};

	bool operator() (typeId leftType, typeId rightType)
	{
		// XXX Check that they are of "typeclass" numerical
		return true;
	}
};

template <binary_op_kind T>
struct ast_binary_valid_dispatch<T, VALID_COMPARABLE>
{
	const universe& u;

	ast_binary_valid_dispatch(const universe& u_) : u(u_) {};

	bool operator() (typeId leftType, typeId rightType)
	{
		// XXX check that they are of "typeclass" comparable
		return true;
	}
};

template <binary_op_kind T>
struct ast_binary_valid_dispatch<T, VALID_LOGICAL>
{
	const universe& u;

	ast_binary_valid_dispatch(const universe& u_) : u(u_) {};

	bool operator() (typeId leftType, typeId rightType)
	{
		return (leftType == u.types().getId("bool").second);
	}
};

template <binary_op_kind T>
struct ast_binary_valid 
{
	const universe& u;
	
	ast_binary_valid(const universe& u_) : u(u_) {};

	bool operator() ( typeId leftType, typeId rightType )
	{
		// only accept operation on same type
		if (leftType == -1 || rightType == -1 ||
			leftType != rightType) 
		{
			return false;
		}

		return ast_binary_valid_dispatch<T, ValidTypeOp<T>::value> (u) ( leftType, rightType );
	}
};

struct ast_valid : public boost::static_visitor<bool>
{
	boost::shared_ptr<ability> pAbility;
	universe& u;

	ast_valid(boost::shared_ptr<ability> pAbility_, universe& u_):
		pAbility(pAbility_), u(u_) 
	{}

	bool operator() (const empty& e) const
	{
		return false;
	}

	template <typename T>
	bool operator() (const Constant<T>& c) const
	{
		return true;
	}

	bool operator() (const std::string& s) const
	{
		std::pair<bool, symbolACL> p;
		p = u.get_symbol(s, pAbility);
		return p.first;
	}

	bool operator() (const function_call& f) const
	{
		// add scope to do the search
		std::string name = u.add_scope(pAbility->name(), f.fName);
		std::pair<bool, functionDef> p = u.get_functionDef(name);
		if (p.first == false) {
			std::cerr << "Unknow function " << f.fName << std::endl;
			return false;
		}

		bool res = true;
		if (f.args.size() != p.second.arity()) {
			std::cerr << "Expected " << p.second.arity() << " arguments";
			std::cerr << " got " << f.args.size() << " for function ";
			std::cerr << f.fName << "!" << std::endl;
			res = false;
		}

		for (size_t i = 0; i < f.args.size(); ++i)
			res = boost::apply_visitor(ast_valid(pAbility, u), f.args[i].expr) && res;

		// check type
		for (size_t i = 0; i < f.args.size(); ++i) {
			typeId id = u.typeOf(pAbility, f.args[i]);
			bool local_res = (id == p.second.argsType(i));
			res = local_res && res;
			if (local_res == false) {
				type expected = u.types().get(p.second.argsType(i));
				std::cerr << "Expected expression of type " << expected.name;
				if (id == -1) {
					std::cerr << " but can't compute type of " << f.args[i].expr;
				} else {
					type local = u.types().get(id);
					std::cerr << " but get " << f.args[i].expr << " of type " << local.name;
				}
				std::cerr << " as argument " << i << " in the call of " << f.fName;
				std::cerr << std::endl;
			}
		}

		return res;
	}

	bool operator() (const expression_ast& e) const
	{
		return boost::apply_visitor(ast_valid(pAbility, u), e.expr);
	}

	template<binary_op_kind T>
	bool operator() (const binary_op<T>& b) const
	{
		bool left_valid, right_valid;
		left_valid = boost::apply_visitor(ast_valid(pAbility, u), b.left.expr);
		right_valid = boost::apply_visitor(ast_valid(pAbility, u), b.right.expr);
		return left_valid && right_valid && ast_binary_valid<T>(u) ( 
													u.typeOf(pAbility, b.left.expr),
													u.typeOf(pAbility, b.right.expr));
	}

	template<unary_op_kind T>
	bool operator() (const unary_op<T>& op) const
	{
		bool is_valid = boost::apply_visitor(ast_valid(pAbility, u), op.subject.expr);
		return is_valid && ast_unary_valid<T>(u) (u.typeOf(pAbility, op.subject.expr)) ;
	}
};

struct cond_adder
{
	bool& res;
	boost::shared_ptr<ability> pAbility;
	universe& u;
	std::vector<expression_ast>& conds;

	cond_adder(bool &res_, boost::shared_ptr<ability> pAbility_, universe& u_,
			std::vector<expression_ast>& conds_):
		res(res_), pAbility(pAbility_), u(u_), conds(conds_)
	{};

	void operator() (const expression_ast& cond) 
	{
		ast_valid is_valid(pAbility, u);
		res = boost::apply_visitor(is_valid, cond.expr) && res;
		conds.push_back(cond);
	}
};

struct task_adder
{
	bool& res;
	boost::shared_ptr<ability> pAbility;
	universe &u;
	task currentTask;

	task_adder(bool &res_, boost::shared_ptr<ability> pAbility_, universe& u_):
		res(res_), pAbility(pAbility_), u(u_)
	{};

	void operator() (const task_decl& t) 
	{
		currentTask.name = t.name;
		{
			cond_adder adder(res, pAbility, u, currentTask.pre);
			std::for_each(t.pre.list.begin(), t.pre.list.end(), adder); 
			res = adder.res && res;
		}
		{
			cond_adder adder(res, pAbility, u, currentTask.post);
			std::for_each(t.post.list.begin(), t.post.list.end(), adder); 
			res = adder.res && res;
		}
		if (res) 
			res = pAbility->add_task(currentTask);
	};
};

bool
universe::add_task(const task_decl_list_context& l)
{
	abilityMap::iterator it = abilities.find(l.ability_name);
	if (it == abilities.end()) {
		std::cerr << "ability " << l.ability_name << " is not declared " << std::endl;
		return false;
	}

	bool res = true;
	task_adder adder(res, it->second, *this);
	std::for_each(l.list.list.begin(), l.list.list.end(), adder);

	return res;
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
	const universe& u;

	compute_deps(std::set<std::string>& s_, const typeList& tlist_, const universe& u_) :
		s(s_), tList(tlist_), u(u_) {};

	void operator() (const std::pair<std::string, symbol> & p) const
	{
		type t = tList.get(p.second.t);
		s.insert(u.get_scope(t.name));
	}
};


struct compute_depends_vis : public boost::static_visitor<void>
{
	std::set<std::string> &s;
	const typeList& tList;
	const universe &u;

	compute_depends_vis(std::set<std::string> &s_, const typeList& tlist_, const universe& u_) : 
		s(s_), tList(tlist_), u(u_) {};

	void operator() (const Nothing& n) const {};

	void operator() (const typeId& t) const
	{
		s.insert(u.get_scope(tList.get(t).name));
	}

	void operator() (const boost::shared_ptr<symbolList> & l) const
	{
		std::for_each(l->begin(), l->end(), compute_deps(s, tList, u));
	}
};

struct compute_depends 
{
	std::set<std::string> &s;
	const typeList& tList;
	const universe& u;

	compute_depends(std::set<std::string>& s_, const typeList& tlist_, const universe& u_) :
		s(s_), tList(tlist_), u(u_) {};

	void operator() (const type& t) const
	{
		boost::apply_visitor(compute_depends_vis(s, tList, u), t.internal);
	}
};

struct dump_depends
{
	std::ostream & oss;
	
	dump_depends(std::ostream& oss_) : oss(oss_) {};

	void operator() (const std::string& s) const
	{
		if (s == "")
			return;

		oss << "#include <hyper/" << s << "/types.hh";
	}
};

struct dump_struct
{
	std::ostream & oss;
	const typeList& tList;

	dump_struct(std::ostream& oss_, const typeList& tList_) : 
		oss(oss_), tList(tList_) {};

	void operator() (const std::pair<std::string, symbol>& p)
	{
		type t = tList.get(p.second.t);
		oss << "\t\t" << t.name << " " << p.first << ";" << std::endl;
	}
};

struct dump_types_vis : public boost::static_visitor<void>
{
	std::ostream & oss;
	const typeList& tList;
	const universe& u;
	std::string name;

	dump_types_vis(std::ostream& oss_, const typeList& tList_, 
				   const universe& u_, const std::string& name_):
		oss(oss_), tList(tList_),u(u_), name(name_) {};

	void operator() (const Nothing& n) const {};

	void operator() (const typeId& tId) const
	{
		type t = tList.get(tId);
		oss << "\ttypedef " << t.name << " " << u.get_identifier(name) << ";";
		oss << "\n" << std::endl;
	}

	void operator() (const boost::shared_ptr<symbolList>& l) const
	{
		oss << "\tstruct " << u.get_identifier(name) << " {" << std::endl;
		std::for_each(l->begin(), l->end(), dump_struct(oss, tList));
		oss << "\t};\n" << std::endl;
	}
};


struct dump_types
{
	std::ostream & oss;
	const typeList& tList;
	const universe& u;

	dump_types(std::ostream& oss_, const typeList& tList_, const universe& u_) :
		oss(oss_), tList(tList_), u(u_) {};

	void operator() (const type& t) const
	{
		boost::apply_visitor(dump_types_vis(oss, tList, u, t.name), t.internal);
	}
};

void
universe::dump_ability_types(std::ostream& oss, const std::string& name) const
{
	// find types prefixed by name::
	std::vector<type> types = tList.select(select_ability_type(name));

	// compute dependances
	std::set<std::string> depends;
	compute_depends deps(depends, tList, *this);
	std::for_each(types.begin(), types.end(), deps);

	oss << "#ifndef _" << name << "_ABILITY_HH_" << std::endl;
	oss << "#define _" << name << "_ABILITY_HH_" << std::endl;

	std::for_each(depends.begin(), depends.end(), dump_depends(oss));

	oss << "namespace hyper { namespace " << name << " {" << std::endl;
	std::for_each(types.begin(), types.end(), dump_types(oss, tList, *this));
	oss << "}; };" << std::endl;
	oss << "#endif /* _" << name << "_ABILITY_HH_ */" << std::endl;
}
