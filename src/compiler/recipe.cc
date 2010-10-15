#include <compiler/ability.hh>
#include <compiler/expression_ast.hh>
#include <compiler/recipe.hh>
#include <compiler/recipe_parser.hh>
#include <compiler/recipe_expression.hh>
#include <compiler/task.hh>
#include <compiler/universe.hh>

using namespace hyper::compiler;

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

struct validate_recipe_expression_ : public boost::static_visitor<bool>
{
	const ability& a;
	const universe& u;
	symbolList& local;

	validate_recipe_expression_(const ability& a_, const universe& u_,
							    symbolList& local_) : 
		a(a_), u(u_), local(local_) {}

	bool operator() (empty) const { assert(false); return false; }

	template <recipe_op_kind kind>
	bool operator() (const recipe_op<kind>& op) const {
		return op.content.is_valid_predicate(a, u, local);
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

namespace hyper {
	namespace compiler {
		recipe::recipe(const recipe_decl& r_parser, const ability& a, 
						const task& t, const typeList& tList_) :
			name(r_parser.name), pre(r_parser.conds.pre.list),
			post(r_parser.conds.post.list), body(r_parser.body.list),
			context_a(a), context_t(t), tList(tList_), local_symbol(tList) 
		{}

		bool recipe::validate(const universe& u) 
							  
		{
			return are_valid_expressions(pre.begin(), pre.end(), context_a, u) &&
				   are_valid_expressions(post.begin(), post.end(), context_a, u) &&
				   are_valid_recipe_expressions(body.begin(), body.end(), context_a, u, local_symbol);
		}

		void recipe::dump(std::ostream& oss, const universe& u) const
		{
			(void) oss; (void) u;
		}
	}
}
