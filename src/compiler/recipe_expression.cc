#include <boost/bind.hpp>

#include <compiler/scope.hh>
#include <compiler/universe.hh>
#include <compiler/recipe_expression.hh>
#include <utils/algorithm.hh>


using namespace hyper::compiler;

std::ostream& hyper::compiler::operator<< (std::ostream& os, const unification_expression& uni)
{
	os << uni.first << " == " << uni.second;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const logic_expression_decl& decl)
{
	os << decl.main;
	if (!decl.unification_clauses.empty()) {
		os << " where ";
		std::copy(decl.unification_clauses.begin(), decl.unification_clauses.end(),
				  std::ostream_iterator<unification_expression>(os, ", "));
	}

	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const let_decl& e)
{
	os << "let " << e.identifier << " = " << e.bounded;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const abort_decl& a)
{
	os << "abort " << a.identifier;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const set_decl& s)
{
	os << "set " << s.identifier << " = " << s.bounded;
	return os;
}

struct recipe_expression_print : public boost::static_visitor<void>
{
	std::ostream& os;

	recipe_expression_print(std::ostream& os_) : os(os_) {}

	template <typename T>
	void operator() (const T& t) const
	{
		os << t;
	}

	void operator() (const empty&) const {}
};

std::ostream& hyper::compiler::operator<< (std::ostream& os, const recipe_expression& e)
{
	boost::apply_visitor(recipe_expression_print(os), e.expr);
	return os;
}

struct extract_destination : public boost::static_visitor<boost::optional<std::string> >
{
	template <typename T>
	boost::optional<std::string> operator() (const T& ) const { return boost::none;}

	boost::optional<std::string> operator() (const std::string& sym) const
	{
		return scope::get_scope(sym);
	}

	boost::optional<std::string> operator() (const function_call& func) const
	{
		size_t i = 0;
		boost::optional<std::string> res = boost::none;
		while (!res && i < func.args.size())
			res = boost::apply_visitor(extract_destination(), func.args[i++].expr);

		return res;
	}

	boost::optional<std::string> operator() (const unary_op& unary) const 
	{
		return boost::apply_visitor(extract_destination(), unary.subject.expr);
	}

	boost::optional<std::string> operator() (const binary_op& binary) const 
	{
		boost::optional<std::string> res = boost::none;
		res = boost::apply_visitor(extract_destination(), binary.left.expr);
		if (!res)
			res = boost::apply_visitor(extract_destination(), binary.right.expr);
		return res;
	}
};

remote_constraint::remote_constraint(const expression_ast& ast) 
{
	logic_expr.main = ast;
	compute_dst();
}

remote_constraint::remote_constraint(const logic_expression_decl& decl) : logic_expr(decl)
{
	compute_dst();
}

void remote_constraint::compute_dst()
{
	dst = boost::apply_visitor(extract_destination(), logic_expr.main.expr);
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const remote_constraint& ctr)
{
	os << ctr.logic_expr;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, recipe_op_kind kind)
{
	switch (kind) 
	{
		case MAKE:
			os << "make";
			break;
		case ENSURE:
			os << "ensure";
			break;
		default:
			assert(false);
	}

	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, observer_op_kind kind)
{
	switch (kind)
	{
		case WAIT:
			os << "wait";
			break;
		case ASSERT:
			os << "assert";
			break;
		default:
			assert(false);
	}

	return os;
}

/* Check if a recipe_expression is valid */
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
		const ability& a, const universe& u, symbolList& local)
{
	return hyper::utils::all(begin, end, boost::bind(&recipe_expression::is_valid, _1, 
													  boost::cref(a), boost::cref(u),
													  boost::ref(local)));
}

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
};

bool recipe_expression::is_valid(const ability &a, const universe& u, symbolList & local) const
{
	return boost::apply_visitor(validate_recipe_expression_(a, u, local), expr);
}
