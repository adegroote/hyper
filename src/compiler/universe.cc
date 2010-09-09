#include <hyperConfig.hh>

#include <set>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

#include <compiler/ability_parser.hh>
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

struct type_diagnostic_visitor : public boost::static_visitor<void>
{
	const type_decl &decl;

	type_diagnostic_visitor(const type_decl& decl_) : decl(decl_) {};

	void operator() (const typeList::native_decl_error& error) const
	{
		(void) error;
		assert(false);
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
			std::cout << "Succesfully adding function " << it1->fName << std::endl;
		} else {
			std::cerr << fList.get_diagnostic(*it1, *it);
		}
	}

	return isOk;
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
universe::get_symbol(const std::string& name, const ability& ab) const
{
	if (!scope::is_scoped_identifier(name)) {
		std::pair<bool, symbolACL> res = ab.get_symbol(name);
		if (res.first == false)
			std::cerr << "Unknow symbol " << name << " in ability " << ab.name() << std::endl;
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
	boost::optional<typeId> id;	
	
	binary_type(const universe& u_, boost::optional<typeId> id_) : u(u_), id(id_) {};

	boost::optional<typeId> operator() () const { return boost::none; };
};

template <binary_op_kind T>
struct binary_type<T, NUMERICAL> {
	const universe& u;
	boost::optional<typeId> id;	
	
	binary_type(const universe& u_, boost::optional<typeId> id_) : u(u_), id(id_) {};

	boost::optional<typeId> operator() () const { return id; };
};

template <binary_op_kind T>
struct binary_type<T, LOGICAL> {
	const universe& u;
	boost::optional<typeId> id;	
	
	binary_type(const universe& u_, boost::optional<typeId> id_) : u(u_), id(id_) {};

	boost::optional<typeId> operator() () const { return u.types().getId("bool").second; };
};

/*
 * Compute the type of an expression
 * We assume that the expression is valid
 */
struct ast_type : public boost::static_visitor<boost::optional<typeId> > {
	const ability& ab;
	const universe& u;

	ast_type(const ability& ab_, const universe& u_):
		ab(ab_), u(u_) 
	{}

	boost::optional<typeId> operator() (const empty& e) const
	{
		(void) e;
		return boost::none;
	}

	boost::optional<typeId> operator() (const Constant<int>& c) const
	{
		(void) c;
		return u.types().getId("int").second;
	}

	boost::optional<typeId> operator() (const Constant<double>& c) const
	{
		(void) c;
		return u.types().getId("double").second;
	}

	boost::optional<typeId> operator() (const Constant<std::string>& c) const
	{
		(void) c;
		return u.types().getId("string").second;
	}

	boost::optional<typeId> operator() (const Constant<bool>& c) const
	{
		(void) c;
		return u.types().getId("bool").second;
	}

	boost::optional<typeId> operator() (const std::string& s) const
	{
		std::pair<bool, symbolACL> p;
		p = u.get_symbol(s, ab);
		if (p.first == false)
			return boost::none;
		return p.second.s.t; 
	}

	boost::optional<typeId> operator() (const function_call& f) const
	{
		// add scope to do the search
		std::string name = scope::add_scope(ab.name(), f.fName);
		std::pair<bool, functionDef> p = u.get_functionDef(name);
		if (p.first == false) 
			return boost::none;

		return p.second.returnType();
	}

	boost::optional<typeId> operator() (const expression_ast& e) const
	{
		return boost::apply_visitor(ast_type(ab, u), e.expr);
	}

	template<binary_op_kind T>
	boost::optional<typeId> operator() (const binary_op<T>& b) const
	{
		boost::optional<typeId> leftId = boost::apply_visitor(ast_type(ab, u), b.left.expr);
		return binary_type<T, TypeOp<T>::value> (u, leftId) ();
	}

	boost::optional<typeId> operator() (const unary_op<NEG>& op) const
	{
		(void) op;
		return u.types().getId("bool").second;
	}
};
	

boost::optional<typeId>
universe::typeOf(const ability& ab, const expression_ast& expr) const
{
	return boost::apply_visitor(ast_type(ab, *this), expr.expr);
}

template <unary_op_kind T>
struct ast_unary_valid 
{
	const universe& u;

	ast_unary_valid(const universe& u_) : u(u_) {};

	bool operator() (boost::optional<typeId> id) const
	{
		return false;
	}
};

template <>
struct ast_unary_valid<NEG>
{
	const universe& u;

	ast_unary_valid(const universe& u_) : u(u_) {};

	bool operator() (boost::optional<typeId> id) const
	{
		if (!id)
			return false;
		return (*id == u.types().getId("int").second || 
				*id == u.types().getId("double").second);
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

	bool operator() (boost::optional<typeId> leftType, boost::optional<typeId> rightType)
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
		(void) leftType;
		(void) rightType;
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
		(void) leftType;
		(void) rightType;
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
		(void) rightType;
		return (leftType == u.types().getId("bool").second);
	}
};

template <binary_op_kind T>
struct ast_binary_valid 
{
	const universe& u;
	
	ast_binary_valid(const universe& u_) : u(u_) {};

	bool operator() (boost::optional<typeId> leftType, boost::optional<typeId> rightType)
	{
		// only accept operation on same type
		if (!leftType || !rightType  || *leftType != *rightType) 
		{
			return false;
		}

		return ast_binary_valid_dispatch<T, ValidTypeOp<T>::value> (u) ( *leftType, *rightType );
	}
};

struct ast_valid : public boost::static_visitor<bool>
{
	const ability& ab;
	universe& u;

	ast_valid(const ability& ab_, universe& u_):
		ab(ab_), u(u_) 
	{}

	bool operator() (const empty& e) const
	{
		(void) e;
		return false;
	}

	template <typename T>
	bool operator() (const Constant<T>& c) const
	{
		(void) c;
		return true;
	}

	bool operator() (const std::string& s) const
	{
		std::pair<bool, symbolACL> p;
		p = u.get_symbol(s, ab);
		return p.first;
	}

	bool operator() (const function_call& f) const
	{
		// add scope to do the search
		std::string name = scope::add_scope(ab.name(), f.fName);
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
			res = boost::apply_visitor(ast_valid(ab, u), f.args[i].expr) && res;

		// check type
		for (size_t i = 0; i < f.args.size(); ++i) {
			boost::optional<typeId> id = u.typeOf(ab, f.args[i]);
			bool local_res = (id == p.second.argsType(i));
			res = local_res && res;
			if (local_res == false) {
				type expected = u.types().get(p.second.argsType(i));
				std::cerr << "Expected expression of type " << expected.name;
				if (!id) {
					std::cerr << " but can't compute type of " << f.args[i].expr;
				} else {
					type local = u.types().get(*id);
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
		return boost::apply_visitor(ast_valid(ab, u), e.expr);
	}

	template<binary_op_kind T>
	bool operator() (const binary_op<T>& b) const
	{
		bool left_valid, right_valid;
		left_valid = boost::apply_visitor(ast_valid(ab, u), b.left.expr);
		right_valid = boost::apply_visitor(ast_valid(ab, u), b.right.expr);
		return left_valid && right_valid && ast_binary_valid<T>(u) ( 
													u.typeOf(ab, b.left.expr),
													u.typeOf(ab, b.right.expr));
	}

	template<unary_op_kind T>
	bool operator() (const unary_op<T>& op) const
	{
		bool is_valid = boost::apply_visitor(ast_valid(ab, u), op.subject.expr);
		return is_valid && ast_unary_valid<T>(u) (u.typeOf(ab, op.subject.expr)) ;
	}
};

struct cond_adder
{
	bool& res;
	const ability& ab;
	universe& u;
	std::vector<expression_ast>& conds;

	cond_adder(bool &res_, const ability& ab_, universe& u_,
			std::vector<expression_ast>& conds_):
		res(res_), ab(ab_), u(u_), conds(conds_)
	{};

	void operator() (const expression_ast& cond) 
	{
		ast_valid is_valid(ab, u);
		res = boost::apply_visitor(is_valid, cond.expr) && res;
		conds.push_back(cond);
	}
};

struct task_adder
{
	bool& res;
	ability& ab;
	universe &u;

	task_adder(bool &res_, ability& ab_, universe& u_):
		res(res_), ab(ab_), u(u_)
	{};

	void operator() (const task_decl& t) 
	{
		task currentTask;
		currentTask.name = t.name;
		{
			cond_adder adder(res, ab, u, currentTask.pre);
			std::for_each(t.pre.list.begin(), t.pre.list.end(), adder); 
			res = adder.res && res;
		}
		{
			cond_adder adder(res, ab, u, currentTask.post);
			std::for_each(t.post.list.begin(), t.post.list.end(), adder); 
			res = adder.res && res;
		}
		if (res) 
			res = ab.add_task(currentTask);
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
	task_adder adder(res, *(it->second), *this);
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


size_t
universe::dump_ability_types(std::ostream& oss, const std::string& name) const
{
	// find types prefixed by name::
	std::vector<type> types = tList.select(select_ability_type(name));

	// compute dependances
	std::set<std::string> depends;
	compute_depends deps(depends, tList);
	std::for_each(types.begin(), types.end(), deps);

	{
		guards g(oss, name, "_TYPE_ABILITY_HH_");

		// include standard needed headers and serialisation header

		oss << "#include <string>\n#include<vector>\n" << std::endl;
		oss << "#include <boost/serialization/serialization.hpp>\n" << std::endl;

		std::for_each(depends.begin(), depends.end(), dump_depends(oss, "types.hh"));

		namespaces n(oss, name);
		std::for_each(types.begin(), types.end(), 
				boost::bind(&type::output, _1, boost::ref(oss), boost::ref(tList)));
	}

	return types.size();
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

size_t
universe::dump_ability_functions_proto(std::ostream& oss, const std::string& name) const
{
	//find functions prefixed by name::
	std::vector<functionDef>  funcs = fList.select(select_ability_funs(name));

	// compute dependances
	std::set<std::string> depends;
	compute_fun_depends deps(depends, tList);
	std::for_each(funcs.begin(), funcs.end(), deps);

	{
		guards g(oss, name, "_FUNC_ABILITY_HH_");


		std::for_each(depends.begin(), depends.end(), dump_depends(oss, "types.hh"));

		oss << "#include <boost/mpl/vector.hpp>\n\n"; 

		namespaces n(oss, name);
		std::for_each(funcs.begin(), funcs.end(), 
				boost::bind(&functionDef::output_proto, _1, boost::ref(oss), boost::ref(tList)));
	}

	return funcs.size();
}

size_t
universe::dump_ability_functions_impl(std::ostream& oss, const std::string& name) const
{
	//find functions prefixed by name::
	std::vector<functionDef>  funcs = fList.select(select_ability_funs(name));

	oss << "#include <" << name << "/funcs.hh>" << std::endl << std::endl;

	namespaces n(oss, name);
	std::for_each(funcs.begin(), funcs.end(), 
			boost::bind(&functionDef::output_impl, _1, boost::ref(oss), boost::ref(tList)));

	return funcs.size();
}

void
universe::dump_ability_import_module_def(std::ostream& oss, const std::string& name) const
{
	oss << "#include <" << name << "/types.hh>" << std::endl;
	oss << "#include <" << name << "/funcs.hh>" << std::endl << std::endl;
	oss << "#include <model/ability.hh>" << std::endl << std::endl;

	namespaces n(oss, name);
	oss << "\t\t\tvoid import_funcs(model::ability &a); " << std::endl;
};

void
universe::dump_ability_import_module_impl(std::ostream& oss, const std::string& name) const
{
	oss << "#include <" << name << "/import.hh>" << std::endl << std::endl;
	oss << "#include <model/execute_impl.hh>" << std::endl << std::endl;

	//find functions prefixed by name::
	std::vector<functionDef>  funcs = fList.select(select_ability_funs(name));

	namespaces n(oss, name);
	oss << "\t\t\tvoid import_funcs(model::ability &a) {" << std::endl;
	std::for_each(funcs.begin(), funcs.end(),
		  boost::bind(&functionDef::output_import, _1, boost::ref(oss)));

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

	it->second->dump(oss, tList, *this);
}

std::set<std::string>
universe::get_function_depends(const std::string& name) const
{
	abilityMap::const_iterator it = abilities.find(name);
	if (it == abilities.end()) {
		std::cerr << "ability " << name << " seems to not be defined ! " << std::endl;
		return std::set<std::string>();
	}

	return it->second->get_function_depends(tList);
}
