#include <logic/expression.hh>

#include <iostream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_algorithm.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

using boost::phoenix::function;
using boost::phoenix::ref;
using boost::phoenix::size;

using namespace hyper::logic;

namespace qi = boost::spirit::qi;
namespace lex = boost::spirit::lex;
namespace ascii = boost::spirit::ascii;
namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;

struct error_handler_
{
    template <typename, typename, typename>
    struct result { typedef void type; };

    template <typename Iterator>
    void operator()(
        qi::info const& what
      , Iterator err_pos, Iterator last) const
    {
        std::cout
            << "Error! Expecting "
            << what                         // what failed?
            << " here: \""
            << std::string(err_pos, last)   // iterators to error-pos, end
            << "\""
            << std::endl
        ;
    }
};

function<error_handler_> const error_handler = error_handler_();

template <typename Lexer>
struct expression_lexer : lex::lexer<Lexer>
{
	expression_lexer() 
	{
		scoped_identifier = "[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*";
		constant_string = "\\\".*\\\"";
		constant_int = "[0-9]+";
		// XXX We don't support local, nor scientific notation atm
		constant_double = "[0-9]*\\.[0-9]*";
		true_ = "true";
		false_ = "false";

		/* identifier must be the last if you want to not match keyword */
        this->self = lex::token_def<>('(') | ')' | ',' ;
		this->self += constant_string;
		this->self += constant_int;
		this->self += constant_double;
		this->self += true_ | false_;
		this->self += scoped_identifier;

        // define the whitespace to ignore (spaces, tabs, newlines and C-style 
        // comments)
        this->self("WS")
            =   lex::token_def<>("[ \\t\\n]+") 
            |   "\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/"
            ;
	};

	lex::token_def<> true_, false_;
    lex::token_def<std::string> scoped_identifier;
	lex::token_def<std::string> constant_string;
	lex::token_def<int> constant_int;
	lex::token_def<double> constant_double;
};

BOOST_FUSION_ADAPT_STRUCT(
		Constant<int>,
		(int, value)
)

BOOST_FUSION_ADAPT_STRUCT(
		Constant<double>,
		(double, value)
)

BOOST_FUSION_ADAPT_STRUCT(
		Constant<std::string>,
		(std::string, value)
)

BOOST_FUSION_ADAPT_STRUCT(
		Constant<bool>,
		(bool, value)
)

BOOST_FUSION_ADAPT_STRUCT(
		function_call,
		(std::string, name)
		(functionId, id)
		(std::vector<expression>, args)
)

struct unknow_func_exception {
	std::string func_name;

	unknow_func_exception(const std::string& func_name_) :
		func_name(func_name_) {};
};

struct bad_arity_exception {
	std::string func_name;
	size_t expected;
	size_t real;

	bad_arity_exception(const std::string& func_name_,
			size_t expected_, size_t real_) :
		func_name(func_name_), expected(expected_), real(real_)
	{}
};

struct validate_func_adaptor {
	const funcDefList& funcs;

	template <typename>
    struct result { typedef function_call type; };

	validate_func_adaptor(const funcDefList& funcs_) : funcs(funcs_) {};

	function_call operator()(const function_call& call) const
	{
		function_call res(call);
		boost::optional<functionId> id = funcs.getId(call.name);
		if (! id )
			throw unknow_func_exception(call.name);
		res.id = *id;

		function_def f = funcs.get(res.id);
		if (f.arity != res.args.size())
			throw bad_arity_exception(f.name, f.arity, res.args.size());

		return res;
	}
};

template <typename Iterator, typename Lexer>
struct grammar_node : qi::grammar<Iterator, expression(), qi::in_state_skipper<Lexer> >
{
    typedef qi::in_state_skipper<Lexer> white_space_;

	template <typename TokenDef>
    grammar_node(const TokenDef& tok) :
		grammar_node::base_type(start, "node")
	{
        using namespace qi::labels;

		using phoenix::at_c;
		using phoenix::val;

		start  = 
				(
				  cst_int				   
				| cst_double			   
				| cst_string			   
				| cst_bool
				| var_inst 			   
				)			   [_val = _1]
				;

		cst_int = tok.constant_int		[at_c<0>(_val) = _1]
				;

		cst_double = tok.constant_double [at_c<0>(_val) = _1]
				;

		cst_string = tok.constant_string [at_c<0>(_val) = _1]
				;

		cst_bool = tok.true_		    [at_c<0>(_val) = true]
				 | tok.false_			[at_c<0>(_val) = false]
				 ;

		var_inst = tok.scoped_identifier [_val = _1]
			;

		start.name("node declaration");
		cst_int.name("const int");
		cst_double.name("const double");
		cst_string.name("const string");
		cst_bool.name("const bool");
		var_inst.name("var instance");

#if 0
		debug(start);
		debug(cst_int);
		debug(cst_double);
		debug(cst_bool);
		debug(var_inst);
#endif

		qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
	}

	qi::rule<Iterator, expression(), white_space_> start;
	qi::rule<Iterator, Constant<int>(), white_space_> cst_int;
	qi::rule<Iterator, Constant<double>(), white_space_> cst_double;
	qi::rule<Iterator, Constant<bool>(), white_space_> cst_bool;
	qi::rule<Iterator, Constant<std::string>(), white_space_> cst_string;
	qi::rule<Iterator, std::string(), white_space_> var_inst;
};

template <typename Iterator, typename Lexer>
struct  grammar_expression : qi::grammar<Iterator, function_call(), qi::in_state_skipper<Lexer> >
{
    typedef qi::in_state_skipper<Lexer> white_space_;

	template <typename TokenDef>
    grammar_expression(const TokenDef& tok, const funcDefList& funcs) : 
		grammar_expression::base_type(func_call, "expression"),
		validate_func(funcs),
		node(tok)
	{
	    using qi::lit;
        using namespace qi::labels;

		using phoenix::at_c;
		using phoenix::val;
		using phoenix::push_back;

		expression_ =
					 func_call						
			        |node							
					;

		func_call = tok.scoped_identifier [at_c<0>(_val) = _1]
				  >> lit('(')
				  >> -(
					   expression_		[push_back(at_c<2>(_val), _1)]
					   >> *( ',' >
						     expression_	[push_back(at_c<2>(_val), _1)])
					  )
				  >> lit(')') [ _val = validate_func(_val)]
				  ;

		expression_.name("expression declaration");
		func_call.name("function declaration instance");

#if 0
		debug(expression_);
		debug(func_call);
#endif

		qi::on_error<qi::fail> (func_call, error_handler(_4, _3, _2));
	}

	qi::rule<Iterator, expression(), white_space_> expression_;
	qi::rule<Iterator, function_call(), white_space_> func_call;

	function<validate_func_adaptor> validate_func;
	grammar_node<Iterator, Lexer> node;
};

struct are_expression_equal : public boost::static_visitor<bool>
{
	template <typename U, typename V>
	bool operator() ( const U& u, const V& v) const
	{
		(void) u; (void) v;
		return false;
	}

	template <typename U>
	bool operator() (const Constant<U>& u, const Constant<U>& v) const
	{
		return (u.value == v.value);
	}

	bool operator() (const std::string& u, const std::string& v) const
	{
		return (u == v);
	}

	bool operator() (const function_call& f1, const function_call& f2) const
	{
		if (f1.id != f2.id)
			return false;
		bool same_args = true;
		size_t i = 0;
		while (same_args && i < f1.args.size())
		{
			same_args = same_args && 
				boost::apply_visitor(are_expression_equal(), f1.args[i].expr, f2.args[i].expr);
			++i;
		}
		return same_args;
	}
};

function_call adapt_local_funcs(const funcDefList& funcs, const function_call& f);

struct adapt_local_funcs_vis : public boost::static_visitor<expression>
{
	const funcDefList& funcs;

	adapt_local_funcs_vis(const funcDefList& funcs) : funcs(funcs) {}

	template <typename T>
	expression operator() (const T& t) const { return t; }

	expression operator() (const function_call& f) const {
		return adapt_local_funcs(funcs, f);
	}
};

function_call adapt_local_funcs(const funcDefList& funcs, const function_call& f)
{
	function_call f_res(f);
	boost::optional<functionId> id = funcs.getId(f.name);
	if (!id) 
		throw unknow_func_exception(f.name);

	f_res.id = *id;
	for (size_t i = 0; i < f_res.args.size(); ++i)
		f_res.args[i] = boost::apply_visitor(adapt_local_funcs_vis(funcs), f.args[i].expr);

	return f_res;
}

namespace hyper {
	namespace logic {
		boost::optional<expression> generate_node(const std::string& expr)
		{
    		typedef std::string::const_iterator base_iterator_type;

    		typedef lex::lexertl::token<
    		    base_iterator_type, boost::mpl::vector<std::string, int , double> 
    		> token_type;

    		// Here we use the lexertl based lexer engine.
    		typedef lex::lexertl::lexer<token_type> lexer_type;

    		// This is the token definition type (derived from the given lexer type).
    		typedef expression_lexer<lexer_type> expression_lexer;

    		// this is the iterator type exposed by the lexer 
    		typedef expression_lexer::iterator_type iterator_type;

    		// this is the type of the grammar to parse
    		typedef grammar_node<iterator_type, expression_lexer::lexer_def> hyper_node;

			expression_lexer our_lexer;
			hyper_node g(our_lexer);

			base_iterator_type it = expr.begin();
			iterator_type iter = our_lexer.begin(it, expr.end());
			iterator_type end = our_lexer.end();
			expression result;
			bool r;
			r  = phrase_parse(iter, end, g, qi::in_state("WS")[our_lexer.self], result);

			if (r && iter == end)
				return result;
			else
				return boost::none;
		}

		generate_return generate(const std::string& expr, const funcDefList& funcs)
		{
    		typedef std::string::const_iterator base_iterator_type;

    		typedef lex::lexertl::token<
    		    base_iterator_type, boost::mpl::vector<std::string, int , double> 
    		> token_type;

    		// Here we use the lexertl based lexer engine.
    		typedef lex::lexertl::lexer<token_type> lexer_type;

    		// This is the token definition type (derived from the given lexer type).
    		typedef expression_lexer<lexer_type> expression_lexer;

    		// this is the iterator type exposed by the lexer 
    		typedef expression_lexer::iterator_type iterator_type;

    		// this is the type of the grammar to parse
    		typedef grammar_expression<iterator_type, expression_lexer::lexer_def> hyper_expression;

			expression_lexer our_lexer;
			hyper_expression g(our_lexer, funcs);

			base_iterator_type it = expr.begin();
			iterator_type iter = our_lexer.begin(it, expr.end());
			iterator_type end = our_lexer.end();
			generate_return result;
			bool r;
			try {
				r  = phrase_parse(iter, end, g, qi::in_state("WS")[our_lexer.self], result.e);
			} catch (unknow_func_exception e)
			{
				std::cerr << "Unknow function used " << e.func_name;
				std::cerr << " : THIS SHOULD NOT HAPPEN !!" << std::endl;
				r = false;
			}
			catch (bad_arity_exception e)
			{
				std::cerr << "Function " << e.func_name << " called with ";
				std::cerr << e.real << " arguments : expected " << e.expected;
				std::cerr << " : THIS SHOULD NOT HAPPEN !!" << std::endl;
				r = false;
			}

			result.res = (r && iter == end);
			return result;
		}

		generate_return generate(const function_call& f, const funcDefList& funcs)
		{
			generate_return result;
			try {
				result.res = true;
				result.e = adapt_local_funcs(funcs, f);
			} catch (unknow_func_exception e) {
				result.res = false;
			}

			return result;
		}

		struct expression_print : public boost::static_visitor<std::string>
		{
			template <typename T>
			std::string operator() (const T& t) const
			{
				std::ostringstream oss;
				oss << t;
				return oss.str();
			}

			std::string operator() (empty e) const
			{
				(void) e;
				return "EMPTY";
			}
		};

		std::ostream& operator << (std::ostream& oss, const expression& e)
		{
			oss << boost::apply_visitor(expression_print(), e.expr);
			return oss;
		}

		std::ostream& operator << (std::ostream& oss, const function_call& f)
		{
			oss << f.name << "(";
			for (size_t i = 0; i < f.args.size(); ++i) {
				oss << f.args[i];
				if (i != (f.args.size() - 1)) 
					oss << ","; 
			}
			oss << ")";
			return oss;
		}

		bool operator == (const expression& e1, const expression& e2)
		{
			return boost::apply_visitor(are_expression_equal(), e1.expr, e2.expr);
		}

		bool operator < (const expression& e1, const expression& e2)
		{
			return e1.expr < e2.expr;
		}

		bool operator < (const function_call& f1, const function_call & f2)
		{
			if (f1.id < f2.id)
				return true;
			if (f1.id > f2.id)
				return false;
			std::vector<expression>::const_iterator it1, it2;
			it1 = f1.args.begin();
			it2 = f2.args.begin();
			while (it1 != f1.args.end())
			{
				if (*it1 < *it2)
					return true;
				if (*it1 == *it2) {
					++it1;
					++it2;
				} else
					return false;
			}

			return false;
		}
			
	}
}
