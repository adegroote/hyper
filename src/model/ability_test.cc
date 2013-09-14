#include <compiler/ability.hh>
#include <compiler/ability_parser.hh>
#include <compiler/recipe_expression.hh>
#include <compiler/scope.hh>

#include <logic/expression.hh>

#include <model/ability_test.hh>
#include <model/actor_impl.hh>
#include <model/future.hh>

#include <boost/random/mersenne_twister.hpp>
#include <boost/version.hpp>
#if BOOST_VERSION < 104700
#include <boost/random/uniform_int.hpp>
#else
#include <boost/random/uniform_int_distribution.hpp>
#endif


template <int N>
std::string make_random_name()
{
	char name[N+1];

	std::string chars(
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"1234567890");
#if BOOST_VERSION < 104700
	boost::uniform_int<> index_dist(0, chars.size() - 1);
	boost::mt19937 gen(std::time(0));
#else
	boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
	boost::random::mt19937 gen(std::time(0));
#endif
	for (size_t i = 0; i < N; ++i)
		name[i] = chars[index_dist(gen)];
	name[N] = 0;

	return name;
}

using namespace hyper::model;

ability_test::ability_test(const std::string& name_) : 
	ability(name_ +  "_test_" + make_random_name<4>(), INFORMATION), 
	target(name_),
	proxy(*this),
	p(u), 
	locals(u.types())
{
	// reference ourself in the universe
	hyper::compiler::ability_decl decl;
	decl.name = this->name;
	u.add(decl);
}

void ability_test::handle_send_constraint(const boost::system::error_code& e,
		network::identifier id,
		future_value<bool> res,
		network::request_constraint2* msg,
		network::request_constraint_answer* ans)
{

	if (e) {
		std::cerr << "Failed to make " << msg->constraint << std::endl;
	} else {
		switch (ans->state) {
			case network::request_constraint_answer::INIT:
			case network::request_constraint_answer::RUNNING:
			case network::request_constraint_answer::PAUSED:
			case network::request_constraint_answer::TEMP_FAILURE:
				/* intermediate reply, do not do anything */
				return;
			case network::request_constraint_answer::SUCCESS:
				std::cerr << "Successfully enforcing " << msg->constraint << std::endl;
				break;
			case network::request_constraint_answer::FAILURE: {
				std::cerr << "Failed to enforce " << msg->constraint << std::endl;
				hyper::network::runtime_failure fail = 
					hyper::network::constraint_failure(msg->constraint);
				fail.error_cause = ans->err_ctx;
				fail.pretty_print(std::cerr);
				break;
				}
			case network::request_constraint_answer::INTERRUPTED:
				std::cerr << "Interrupted while enforcing " << msg->constraint << std::endl;
				break;
		}
	}
	actor->db.remove(id);

	res.get_raw() = (!e && 
					 ans->state == network::request_constraint_answer::SUCCESS);
	res.signal_ready();

	delete msg;
	delete ans;

	stop();
}


struct generate_logic_function_ : public boost::static_visitor<hyper::logic::expression>
{
	const hyper::compiler::universe &u;
	const hyper::compiler::ability& a;

	generate_logic_function_(const hyper::compiler::universe& u, 
							 const hyper::compiler::ability& a) : 
		u(u), a(a) {}

	template <typename T>
	hyper::logic::expression operator() (const hyper::compiler::Constant<T>& value) const
	{
		return hyper::logic::Constant<T>(value.value);
	}

	hyper::logic::expression operator() (const hyper::compiler::binary_op& op) const
	{
		boost::optional<hyper::compiler::typeId> tid = u.typeOf(a, op.left.expr);
		assert(tid);
		if (tid) {
			hyper::compiler::type t = u.types().get(*tid);
			std::string fun_name = logic_function_name(op.op) + "_" + t.name;
			return hyper::logic::function_call(
				fun_name,
				boost::apply_visitor(*this, op.left.expr),
				boost::apply_visitor(*this, op.right.expr));
		}	

		return hyper::logic::empty();
	}

	hyper::logic::expression operator() (const hyper::compiler::unary_op& op) const
	{
		assert(false);
	}

	hyper::logic::expression operator() (const std::string& s) const 
	{ 
		if (!hyper::compiler::scope::is_scoped_identifier(s))
			return a.name() + "::" + s;
		else
			return s;
	}

	hyper::logic::expression operator() (const hyper::compiler::expression_ast& e) const
	{
		return boost::apply_visitor(*this, e.expr);
	}

	hyper::logic::expression operator() (const hyper::compiler::function_call& f) const
	{
		hyper::logic::function_call res;
		res.name = f.fName;
		res.args.resize(f.args.size());
		for (size_t i = 0; i < f.args.size(); ++i) 
			res.args[i] = boost::apply_visitor(*this, f.args[i].expr);
		return res;
	}
};

hyper::logic::function_call
generate_logic_function(const hyper::compiler::expression_ast& f, 
						const hyper::compiler::universe& u,
						const hyper::compiler::ability& a)
{
	hyper::logic::expression e = boost::apply_visitor(generate_logic_function_(u, a), f.expr);
	return boost::get<hyper::logic::function_call>(e.expr);
}

template <hyper::compiler::recipe_op_kind kind>
void generate_request(const hyper::compiler::recipe_expression& expr, 
					  hyper::network::request_constraint2& msg,
					  const hyper::compiler::universe& u,
					  const hyper::compiler::ability& a)
{
	hyper::compiler::recipe_op<kind> r = boost::get<hyper::compiler::recipe_op<kind> >(expr.expr);
	msg.constraint = generate_logic_function(r.content[0].logic_expr.main, u, a);
	msg.unify_list.clear();
	const std::vector<hyper::compiler::unification_expression>& v = 
		r.content[0].logic_expr.unification_clauses;
	msg.unify_list.reserve(v.size());
	for (size_t i = 0; i < v.size(); ++i) {
		msg.unify_list.push_back(std::make_pair(
				boost::apply_visitor(generate_logic_function_(u, a), v[i].first.expr),
				boost::apply_visitor(generate_logic_function_(u, a), v[i].second.expr)));
	}
}

future_value<bool>
ability_test::send_constraint(const hyper::compiler::recipe_expression& expr, bool repeat)
{
	network::request_constraint2* msg(new network::request_constraint2());
	network::request_constraint_answer *answer(new network::request_constraint_answer());
	msg->repeat = repeat;
	if (repeat) {
		generate_request<hyper::compiler::ENSURE>(expr, *msg, u, u.get_ability(name));
        msg->delay = 50.0;
	} else {
		generate_request<hyper::compiler::MAKE>(expr, *msg, u, u.get_ability(name));
	}

	future_value<bool> res("");

	actor->client_db[target].async_request(*msg, *answer, 
			boost::bind(&ability_test::handle_send_constraint,
				this,
				boost::asio::placeholders::error,
				_2, res, msg, answer));

	return res;
}

bool
ability_test::correct_usage(int argc, char** argv) 
{
    if (argc != 3 && argc != 7)
        return false;

	std::vector<std::string> arguments(argv + 1, argv + argc);

	return (arguments[0] == "get" or
            arguments[0] == "make" or
		    arguments[0] == "ensure" or
		    (arguments[0] == "let" and argc == 7  and
                    (arguments[4] != "make" or arguments[4] == "ensure")));
}

int 
ability_test::usage(const std::string& name) 
{
	std::string prog_name = "hyper_" + name + "_test";
	std::cerr << "Usage :\n";
	std::cerr << prog_name << " get <var_name>\n";
	std::cerr << prog_name << "[let var type json_value] make <request>\n";
	std::cerr << prog_name << "[let var type json_value] ensure <request>\n";
	return -1;
}

int ability_test::main(int argc, char ** argv)
{
	using namespace hyper::compiler;
	std::vector<std::string> arguments(argv + 1, argv + argc);
	boost::function<void ()> to_execute;

	if (arguments[0] == "get") {
		getter_map::const_iterator it;
		it = gmap.find(arguments[1]);
		if (it != gmap.end()) {
			to_execute = it->second;
		} else {
			std::cerr << target << " does not have such variable: " << arguments[1] << std::endl;
			return -1;
		}
	}
	
	int cmd_index = 0;
	if (arguments[0] == "let") {
		// parse the file to get the definition of all necessary types
		bool res = p.parse_ability_file(target + ".ability");
		if (!res) {
			std::cerr << "Failed to parse properly " << target << ".ability" << std::endl;
			return -1;
		}

		factory_map::const_iterator it = fmap.find(arguments[2]);
		if (it == fmap.end()) {
			std::cerr << "Can't find factory for type " << arguments[2] << std::endl;
			return -1;
		}

		res = it->second(arguments[1], arguments[3]);
		if (!res) {
			std::cerr << "Unexpected failure in factory of " << arguments[1] << std::endl;
			return -1;
		}
		cmd_index = 4;
	}

	if (arguments[cmd_index] == "make" or arguments[cmd_index] == "ensure")
	{
		// if we have already enter in let, this call will do nothing
		bool res = p.parse_ability_file(target + ".ability");
		if (!res) {
			std::cerr << "Failed to parse properly " << target << ".ability" << std::endl;
			return -1;
		}

		logic_expression_decl decl;
		res = p.parse_logic_expression(arguments[cmd_index + 1], decl);

		if (!res) {
			std::cerr << "Failed to parse properly " << arguments[cmd_index + 1] << std::endl;
			return -1;
		}

		recipe_expression r;
		if (arguments[cmd_index] == "make")
			r = recipe_op<MAKE>(decl);
		else
			r = recipe_op<ENSURE>(decl);


		const hyper::compiler::ability &a = u.get_ability(this->name);
		res = r.is_valid(a, u, locals);
		if (!res) return -1;

		to_execute = boost::bind(&ability_test::send_constraint, this,
								    r, (arguments[cmd_index] == "ensure"));
	}

	register_name();
	to_execute();
	run();

	return 0;
}
