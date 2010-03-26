#include <fstream>
#include <iostream>
#include <sstream>

#include <compiler/expression.hh>
#include <compiler/parser.hh>

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

//#define HYPER_DEBUG_RULES

using boost::phoenix::function;
using boost::phoenix::ref;
using boost::phoenix::size;

using namespace hyper::compiler;

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
struct hyper_lexer : lex::lexer<Lexer>
{
	hyper_lexer() 
	{
        identifier = "[a-zA-Z_][a-zA-Z0-9_]*";
		scoped_identifier = "[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*";
		constant_string = "\\\".*\\\"";
		constant_int = "[0-9]+";
		// XXX We don't support local, nor scientific notation atm
		constant_double = "[0-9]*\\.[0-9]*";

		/* identifier must be the last if you want to not match keyword */
        this->self = lex::token_def<>('(') | ')' | '{' | '}' | '=' | ';' | ',' ;
		this->self += identifier | scoped_identifier;
		this->self += constant_string;
		this->self += constant_int;
		this->self += constant_double;

        // define the whitespace to ignore (spaces, tabs, newlines and C-style 
        // comments)
        this->self("WS")
            =   lex::token_def<>("[ \\t\\n]+") 
            |   "\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/"
            ;
	};

    lex::token_def<std::string> identifier, scoped_identifier;
	lex::token_def<std::string> constant_string;
	lex::token_def<int> constant_int;
	lex::token_def<double> constant_double;
};

BOOST_FUSION_ADAPT_STRUCT(
		Constant<int>,
		(int, value)
);

BOOST_FUSION_ADAPT_STRUCT(
		Constant<double>,
		(double, value)
);

BOOST_FUSION_ADAPT_STRUCT(
		Constant<std::string>,
		(std::string, value)
);

BOOST_FUSION_ADAPT_STRUCT(
		function_call,
		(std::string, fName)
		(std::vector<node>, args)
);


template <typename Iterator, typename Lexer>
struct  grammar_expression : qi::grammar<Iterator, node(), qi::in_state_skipper<Lexer> >
{
    typedef qi::in_state_skipper<Lexer> white_space_;

	template <typename TokenDef>
    grammar_expression(const TokenDef& tok) : 
		grammar_expression::base_type(node_, "node")
	{
	    using qi::lit;
        using qi::lexeme;
        using ascii::char_;
        using namespace qi::labels;

		using phoenix::at_c;
        using phoenix::push_back;
		using phoenix::insert;
		using phoenix::begin;
		using phoenix::end;
		using phoenix::swap;
		using phoenix::construct;
		using phoenix::val;
		
		node_ = (
				  func_call
				| cst_int				   
				| cst_double			   
				| cst_string			   
				| var_inst 			   
				)			   [_val = _1]
			 ;

		cst_int = tok.constant_int		[at_c<0>(_val) = _1]
				;

		cst_double = tok.constant_double [at_c<0>(_val) = _1]
				;

		cst_string = tok.constant_string [at_c<0>(_val) = _1]
				;

		var_inst = tok.identifier		[_val = _1]
			;

		func_call = tok.identifier		[at_c<0>(_val) = _1]
				  >> lit('(')
				  >> -(
					   node_			[push_back(at_c<1>(_val), _1)]
					   >> *( lit(',') >
						     node_			[push_back(at_c<1>(_val), _1)])
					  )
				  >> lit(')')
				  ;

		node_.name("node declaration");
		cst_int.name("const int");
		cst_double.name("const double");
		cst_string.name("const string");
		var_inst.name("var instance");
		func_call.name("function declaration instance");

		qi::on_error<qi::fail> (node_, error_handler(_4, _3, _2));

	};

	qi::rule<Iterator, node(), white_space_> node_;
	qi::rule<Iterator, Constant<int>(), white_space_> cst_int;
	qi::rule<Iterator, Constant<double>(), white_space_> cst_double;
	qi::rule<Iterator, Constant<std::string>(), white_space_> cst_string;
	qi::rule<Iterator, std::string(), white_space_> var_inst;
	qi::rule<Iterator, function_call(), white_space_> func_call;

};

bool parser::parse_expression(const std::string& expr)
{
    // This is the lexer token type to use. The second template parameter lists 
    // all attribute types used for token_def's during token definition (see 
    // calculator_tokens<> above). Here we use the predefined lexertl token 
    // type, but any compatible token type may be used instead.
    //
    // If you don't list any token attribute types in the following declaration 
    // (or just use the default token type: lexertl_token<base_iterator_type>)  
    // it will compile and work just fine, just a bit less efficient. This is  
    // because the token attribute will be generated from the matched input  
    // sequence every time it is requested. But as soon as you specify at 
    // least one token attribute type you'll have to list all attribute types 
    // used for token_def<> declarations in the token definition class above, 
    // otherwise compilation errors will occur.

    typedef std::string::const_iterator base_iterator_type;

    typedef lex::lexertl::token<
        base_iterator_type, boost::mpl::vector<std::string, int , double> 
    > token_type;

    // Here we use the lexertl based lexer engine.
    typedef lex::lexertl::lexer<token_type> lexer_type;

    // This is the token definition type (derived from the given lexer type).
    typedef hyper_lexer<lexer_type> hyper_lexer;

    // this is the iterator type exposed by the lexer 
    typedef hyper_lexer::iterator_type iterator_type;

    // this is the type of the grammar to parse
    typedef grammar_expression<iterator_type, hyper_lexer::lexer_def> hyper_expression;

	hyper_lexer our_lexer;
	hyper_expression g(our_lexer);

	base_iterator_type it = expr.begin();
	iterator_type iter = our_lexer.begin(it, expr.end());
	iterator_type end = our_lexer.end();
	node result;
    bool r = phrase_parse(iter, end, g, qi::in_state("WS")[our_lexer.self], result);

	if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";

		std::cout << "Node : " << result << std::endl;
        return true;
    }
    else
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "-------------------------\n";
        return false;
    }

}	


