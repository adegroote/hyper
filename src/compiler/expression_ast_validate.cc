#include <compiler/ability.hh>
#include <compiler/expression_ast.hh>
#include <compiler/scope.hh>
#include <compiler/types.hh>
#include <compiler/symbols.hh>
#include <compiler/universe.hh>

using namespace hyper::compiler;

struct ast_unary_valid 
{
	const universe& u;
	const unary_op& expr;

	ast_unary_valid(const universe& u_, const unary_op& expr_) : u(u_), expr(expr_) {};

	bool operator() (boost::optional<typeId> id) const
	{
		if (!id)
			return false;

		switch(expr.op) {
			case NEG:
				return (*id == u.types().getId("int").second || 
						*id == u.types().getId("double").second);
			default:
				return false;
		}
	}
};

struct ast_binary_valid 
{
	const universe& u;
	const binary_op& expr;
	
	ast_binary_valid(const universe& u_, const binary_op& expr) : u(u_), expr(expr) {};

	bool operator() (boost::optional<typeId> leftType, boost::optional<typeId> rightType)
	{
		if (!leftType) {
			std::cerr << "Can't get typeOf of " << expr.left << " in expression " << expr << std::endl;
			return false;
		}

		if (!rightType) {
			std::cerr << "Can't get typeOf of " << expr.right << " in expression " << expr << std::endl;
			return false;
		}

		if (*leftType != *rightType) 
		{
			type l = u.types().get(*leftType);
			type r = u.types().get(*rightType);

			std::cerr << "Left operand of type " << l.name << " mismatches with right operand of type " << r.name;
			std::cerr << " in expression " << expr << std::endl;
			return false;
		}

		switch(expr.op) {
			// numerical operation
			case ADD:
			case SUB:
			case MUL:
			case DIV:
				return true; // XXX check that there are of kind numerical

			// logical operation
			case AND:
			case OR:
				return (*leftType == u.types().getId("bool").second);

			case GT:
			case GTE:
			case LT:
			case LTE:
			case EQ:
			case NEQ:
				return true;

			default:
				assert(false);
		}
	}
};

struct ast_valid : public boost::static_visitor<bool>
{
	const ability& ab;
	const universe& u;
	const boost::optional<symbolList>& context;
	bool first; /* check if a function call with a tag is called, and where */

	ast_valid(const ability& ab_, const universe& u_,
			  const boost::optional<symbolList>& context_,
			  bool first):
		ab(ab_), u(u_), context(context_), first(first)
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
		p = u.get_symbol(s, ab, context);
		if (!p.first) 
			std::cerr << "Can't find symbol " << s << " in the current context " << std::endl;
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

		const boost::optional<std::string>& tag = p.second.tag();
		if (tag && !first) {
			std::cerr << "Tagged function are only supported as the first element of an expression";
			std::cerr << std::endl;
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
			res = boost::apply_visitor(ast_valid(ab, u, context, false), f.args[i].expr) && res;

		// check type
		for (size_t i = 0; i < f.args.size(); ++i) {
			boost::optional<typeId> id = u.typeOf(ab, f.args[i], context);
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
		return boost::apply_visitor(ast_valid(ab, u, context, false), e.expr);
	}

	bool operator() (const binary_op& b) const
	{
		bool left_valid, right_valid;
		left_valid = boost::apply_visitor(ast_valid(ab, u, context, false), b.left.expr);
		right_valid = boost::apply_visitor(ast_valid(ab, u, context, false), b.right.expr);
		return left_valid && right_valid && ast_binary_valid(u, b) ( 
													u.typeOf(ab, b.left.expr, context),
													u.typeOf(ab, b.right.expr, context));
	}

	bool operator() (const unary_op& op) const
	{
		bool is_valid = boost::apply_visitor(ast_valid(ab, u, context, false), op.subject.expr);
		return is_valid && ast_unary_valid(u, op) (u.typeOf(ab, op.subject.expr, context)) ;
	}
};

namespace hyper {
	namespace compiler {
		bool expression_ast::is_valid(const ability& context, const universe& u,
									  const boost::optional<symbolList>& local_context) const
		{
			ast_valid is_valid(context, u, local_context, true);
			return boost::apply_visitor(is_valid, this->expr);
		}

		bool expression_ast::is_valid_predicate(const ability& context, const universe& u,
									const boost::optional<symbolList>& local_context) const
		{
			bool valid_expression = is_valid(context, u, local_context);
			std::pair<bool, typeId> expected_type = u.types().getId("bool");
			assert(expected_type.first);
			boost::optional<typeId> real_type = u.typeOf(context, *this);
			bool valid_type = real_type && (*real_type == expected_type.second);

			return valid_expression && valid_type;
		}
	}
}
