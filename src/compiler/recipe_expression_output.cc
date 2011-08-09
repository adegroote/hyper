#include <hyperConfig.hh>

#include <compiler/extension.hh>
#include <compiler/logic_expression_output.hh>
#include <compiler/output.hh>
#include <compiler/recipe_expression.hh>
#include <compiler/recipe_expression_output.hh>
#include <compiler/universe.hh>


namespace {
	using namespace hyper::compiler;

	struct prepare_logic_symbol_ : public boost::static_visitor<expression_ast>
	{
		const symbolList& sym;
		const task& t;

		prepare_logic_symbol_(const symbolList& sym, const task& t) : sym(sym), t(t) {}

		template <typename T>
		expression_ast operator() (const T& t) const { return t; }

		expression_ast operator() (const std::string& s) const {
			std::pair<bool, symbol> p = sym.get(s);
			if (p.first) 
				return exported_symbol(s, t);
			else
				return s;
		}

		expression_ast operator() (const function_call& f) const {
			function_call res(f);
			for (size_t i = 0; i < f.args.size(); ++i)
				res.args[i] = boost::apply_visitor(*this, f.args[i].expr);
			return res;
		}

		expression_ast operator() (const expression_ast& e) const {
			return boost::apply_visitor(*this, e.expr);
		}

		template <unary_op_kind T>
		expression_ast operator() (const unary_op<T>& op) const {
			unary_op<T> res(op);
			res.subject = boost::apply_visitor(*this, op.subject.expr);
			return res;
		}

		template <binary_op_kind T>
		expression_ast operator() (const binary_op<T>& op) const {
			binary_op<T> res(op);
			res.left = boost::apply_visitor(*this, op.left.expr);
			res.right = boost::apply_visitor(*this, op.right.expr);
			return res;
		}
	};

	struct prepare_logic_symbol_unification {
		prepare_logic_symbol_ vis;

		prepare_logic_symbol_unification(const symbolList& sym, const task& t) : vis(sym, t) {}

		unification_expression operator() (const unification_expression& p) const
		{
			return std::make_pair( boost::apply_visitor(vis, p.first.expr),
								   boost::apply_visitor(vis, p.second.expr));
		}
	};


	logic_expression_decl prepare_logic_symbol(const logic_expression_decl& decl,
											   const symbolList& sym, const task& t)
	{
		logic_expression_decl res(decl);
		res.main = boost::apply_visitor(prepare_logic_symbol_(sym, t), decl.main.expr);
		std::transform(res.unification_clauses.begin(), res.unification_clauses.end(),
					   res.unification_clauses.begin(), prepare_logic_symbol_unification(sym, t));
		return res;
	}

	struct dump_unify_pair {
		std::ostream& os;
		std::string separator_;
		const ability & a;
		const universe& u;

		dump_unify_pair(std::ostream& os, const ability& a, const universe& u) : 
			os(os), a(a), u(u) 
		{
			separator_ = "\n" + times(5, "\t");
		}

		void operator() (const unification_expression& p) 
		{
			os << "(" << quoted_string(generate_logic_expression(p.first.expr, a, u));
			os << ", " << quoted_string(generate_logic_expression(p.second.expr, a, u));
			os << ")";
		};

		void separator () { 
			os << separator_;
		}
	};

	struct dump_recipe_visitor : public boost::static_visitor<std::string>
	{
		const universe& u;
		const ability &a;
		const task& t;
		const symbolList& syms;
		mutable size_t counter;
		mutable boost::optional<std::string> target;

		dump_recipe_visitor(const universe & u_, const ability& a_, const task& t_, 
						   const symbolList& syms_) : 
			u(u_), a(a_), t(t_), syms(syms_), counter(0), target(boost::none) {}

		template <typename T> 
		std::string operator() (const T&) const { return ""; }

		std::string local_symbol(const std::string& s) const 
		{
			return "local_vars." + s;
		}

		std::string unused_symbol(const std::string& t) const
		{
			return "unused_res." + mangle(t);
		}

		std::string compute_target(const expression_ast& e) const
		{
			if (target) {
				std::pair<bool, symbolACL> p = a.get_symbol(*target);
				if (p.first) 
					return "a." + *target;
				else 
					return local_symbol(*target);
			} else {
				boost::optional<typeId> tid = u.typeOf(a, e, syms);
				type t = u.types().get(*tid);
				return unused_symbol(t.type_name());
			}
		}

		std::string compute_make_target() const
		{
			if (target) 
				return local_symbol(*target);
			else 
				return unused_symbol("bool");
		}

		std::string compute_ensure_target() const
		{
			if (target) 
				return local_symbol(*target);
			else 
				return unused_symbol("hyper::model::identifier");
		}

		std::string abortable_function(const std::string& identifier, const expression_ast& e) const
		{
			std::pair<bool, std::string> p = is_tagged_func_call(e, a.name(), u);

			if (p.first) 
				return u.get_extension(p.second).generate_abortable_function(
						u, a, t, "local_context", counter++, identifier);

			std::ostringstream oss;
			std::string indent = times(3, "\t");
			std::string next_indent = "\t" + indent;

			oss << next_indent << "new hyper::model::abortable_function(\n";
			oss << next_indent << "boost::bind(&hyper::model::compute_expression<expression_" << counter << ">::async_eval<\n";
			oss << next_indent << "hyper::model::abortable_computation::cb_type,\n";
			oss << next_indent << "hyper::" << a.name() << "::ability, local_context>,\n";
			oss << next_indent << "&expression_exec" << counter++ << ", _1, boost::cref(a), boost::cref(local_vars),\n";
			oss << next_indent << "boost::ref(" << identifier << ")))\n";

			return oss.str();
		}

		std::string operator() (const let_decl& s) const
		{
			target = s.identifier;
			return boost::apply_visitor(*this, s.bounded.expr);
		}

		std::string dump_unification(const std::vector<unification_expression>& unify_list) const
		{
			std::ostringstream oss;
			if (unify_list.empty()) 
				oss << "network::request_constraint::unification_list()";
			else {
				oss << "boost::assign::list_of<std::pair<std::string, std::string> >";
				oss << "\n" << times(5, "\t");
				hyper::compiler::list_of(unify_list.begin(), unify_list.end(), dump_unify_pair(oss, a, u));
				oss << "\n";
			}

			return oss.str();
		}

		std::string operator() (const wait_decl& w) const
		{
			std::string indent = times(4, "\t");
			std::string identifier = compute_target(w.content);
			std::string next_indent = "\t" + indent;

			std::ostringstream oss;
			oss << indent << "push_back(new hyper::model::compute_wait_expression(a.io_s, boost::posix_time::milliseconds(50), \n";
			oss << abortable_function(identifier, w.content) << ",";
			oss << next_indent << identifier << "));\n";

			target = boost::none;

			return oss.str();
		}

		std::string operator() (const expression_ast& e) const
		{
			std::string indent = times(4, "\t");
			std::string identifier = compute_target(e);

			std::ostringstream oss;
			oss << indent << "push_back(" << abortable_function(identifier, e) << indent << ");\n";

			target = boost::none;

			return oss.str();
		}

		std::string operator() (const set_decl& s) const
		{
			std::string indent = times(4, "\t");

			target = s.identifier;
			std::string identifier = compute_target(s.bounded.expr);

			std::ostringstream oss;
			oss << indent << "push_back(" << abortable_function(identifier, s.bounded.expr) << indent << ");\n";
			target = boost::none;

			return oss.str();
		}

		std::string operator() (const recipe_op<MAKE>& r) const
		{
			std::string indent = times(4, "\t");
			std::string indent_next = times(5, "\t");
			std::string identifier = compute_make_target();
			logic_expression_decl local_expr = prepare_logic_symbol(r.content[0].logic_expr, syms, t);

			std::ostringstream oss;

			oss << indent << "push_back(new hyper::model::compute_make_expression(a, ";
			oss << quoted_string(*(r.content[0].dst)) << ", ";
			oss << quoted_string(generate_logic_expression(local_expr.main, a, u));
			oss << ", \n" << indent_next << dump_unification(local_expr.unification_clauses);
			oss << ", " << identifier << "));\n";
			target = boost::none;

			return oss.str();

		}

		std::string operator() (const recipe_op<ENSURE>& r) const
		{
			std::string indent = times(4, "\t");
			std::string indent_next = times(5, "\t");
			std::string identifier = compute_ensure_target();
			logic_expression_decl local_expr = prepare_logic_symbol(r.content[0].logic_expr, syms, t);
			std::ostringstream oss;

			oss << indent << "push_back(new hyper::model::compute_ensure_expression(a, ";
			oss << quoted_string(*(r.content[0].dst)) << ", \n" << indent_next;
			oss << quoted_string(generate_logic_expression(local_expr.main, a, u));
			oss << ", \n" << indent_next << dump_unification(local_expr.unification_clauses);
			oss << ", \n" << indent_next << identifier << "));\n";
			target = boost::none;

			return oss.str();
		}

		std::string operator() (const abort_decl& a) const
		{
			std::string indent = times(4, "\t");
			std::string indent_next = times(5, "\t");
			std::ostringstream oss;

			oss << indent << "push_back(new hyper::model::compute_abort_expression(a, \n";
			oss << indent_next << local_symbol(a.identifier) << ", \n";
			oss << indent_next << unused_symbol("boost::mpl::void_") << "));";
			target = boost::none;

			return oss.str();
		}
	};
}

namespace hyper {
	namespace compiler {
		std::string mangle(const std::string& s)
		{
			return "__hyper_" + hyper::compiler::replace_by(s, "::", "__");
		}

		std::string exported_symbol(const std::string& s, const task& t)
		{
			return t.exported_name() + "_" + s;
		}

		void dump_recipe_expression::operator() (const recipe_expression& r) const
		{
			dump_recipe_visitor vis(u, a, t, syms);
			oss << boost::apply_visitor(vis, r.expr);
		}
	}
}
