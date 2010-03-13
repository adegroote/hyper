
#include <compiler/parser.hh>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

using boost::phoenix::function;
using boost::phoenix::ref;
using boost::phoenix::size;

using namespace hyper::compiler;


namespace qi = boost::spirit::qi;
namespace lex = boost::spirit::lex;
namespace ascii = boost::spirit::ascii;
namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;

parser::parser() : tList(), sList(tList), fList(tList)
{
	// Add basic native types
	
	tList.add("bool", boolType);
	tList.add("double", doubleType);
	tList.add("int", intType);
	tList.add("string", stringType);
	tList.add("void", noType);
}


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
		scoped_identifier = "[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_])*";
		struct_ = "struct";
		newtype_ = "newtype";

		/* identifier must be the last if you want to not match keyword */
        this->self = lex::token_def<>('(') | ')' | '{' | '}' | '=' | ';' | ',';
		this->self += struct_ | newtype_ | identifier | scoped_identifier;

        // define the whitespace to ignore (spaces, tabs, newlines and C-style 
        // comments)
        this->self("WS")
            =   lex::token_def<>("[ \\t\\n]+") 
            |   "\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/"
            ;
	};

    lex::token_def<> struct_, newtype_;
    lex::token_def<std::string> identifier, scoped_identifier;
};

struct var_decl {
	std::string type;
	std::string sym;
};

BOOST_FUSION_ADAPT_STRUCT(
    var_decl,
    (std::string, type)
	(std::string, sym)
);

struct symbol_adder {
	symbolList & s;

	template <typename>
    struct result { typedef void type; };
	
	symbol_adder(symbolList & s_) : s(s_) {};
	void operator()(const var_decl & decl) const
	{
		std::pair < bool, symbolList::symbolAddError > p;
		p = s.add(decl.sym, decl.type);
		if ( p.first == false ) {
			switch ( p.second ) {
				case symbolList::alreadyExist: 
					std::cerr << "variable " << decl.sym << " already defined" << std::endl;
					break;
				case symbolList::unknowType:
					std::cerr << "type " << decl.type << " used to declare " << decl.sym;
					std::cerr << " is undefined" << std::endl;
					break;
				case symbolList::noError:
				default:
					;
			}
		} else {
			std::cout << "Successfully add var " << decl.sym << std::endl;
		}
	};
};

struct function_decl {
	std::string fName;
	std::string returnName;
	std::vector < std::string > argsName;
};

struct function_adder {

	functionDefList & f;

	template<typename>
	struct result { typedef void type; };

	function_adder(functionDefList & fList) : f(fList) {};
	void operator()(const function_decl & decl) const
	{
		boost::tuple< bool, functionDefList::addErrorType, int> r;
		r = f.add(decl.fName, decl.returnName, decl.argsName);
		if ( r.get<0>() == false ) {
			switch (r.get<1>()) {
				case functionDefList::alreadyExist:
					std::cerr << "function " << decl.fName << " is already defined " << std::endl;
					break;
				case functionDefList::unknowReturnType:
					std::cerr << "type " << decl.returnName << " is used in definition of ";
					std::cerr << decl.fName << " but it nos defined " << std::endl;
					break;
				case functionDefList::unknowArgsType:
					std::cerr << "type " << decl.argsName[r.get<2>()] << " is used in definition of ";
					std::cerr << decl.fName << " but it nos defined " << std::endl;
					break;
				case functionDefList::noError:
				default:
					;
			}
		} else {
			std::cout << "Succesfully add definition function for " << decl.fName << std::endl;
		}
	};
};

BOOST_FUSION_ADAPT_STRUCT(
    function_decl,
    (std::string, fName)
	(std::string, returnName)
	(std::vector < std::string>, argsName)
);

struct struct_decl {
	std::string name;
	std::vector < var_decl > vars;
};

BOOST_FUSION_ADAPT_STRUCT(
	struct_decl,
	(std::string, name)
	(std::vector < var_decl >, vars)
);

struct struct_adder {
	template<typename>
	struct result { typedef void type; };

	void operator() (const struct_decl& s) const 
	{
		std::cout << "struct " << s.name << " contains :" << std::endl;
		for (size_t i = 0; i < s.vars.size(); ++i)
			std::cout << "\t" << s.vars[i].type << " " << s.vars[i].sym << std::endl;
	}
};

struct newtype_decl {
	std::string newname;
	std::string oldname;
};

BOOST_FUSION_ADAPT_STRUCT(
	newtype_decl,
	(std::string, newname)
	(std::string, oldname)
);

struct newtype_adder {
	template<typename>
	struct result { typedef void type; };

	void operator() (const newtype_decl& s) const 
	{
		std::cout << "type " << s.newname << " is synonym of " << s.oldname << std::endl;
	};
};

template <typename Iterator, typename Lexer>
struct expression : qi::grammar<Iterator, qi::in_state_skipper<Lexer> >
{
    typedef qi::in_state_skipper<Lexer> white_space_;

	template <typename TokenDef>
    expression(const TokenDef& tok, symbolList &s, functionDefList& f) :
								expression::base_type(statement_list, "statement_list"),
								symbol_add(s),
								function_decl_add(f),
								struct_decl_add()
	{
	    using qi::lit;
        using qi::lexeme;
        using ascii::char_;
        using namespace qi::labels;

		using phoenix::at_c;
        using phoenix::push_back;

		v_decl   = tok.identifier			[at_c<0>(_val) = _1]
				 >> tok.identifier			[at_c<1>(_val) = _1]
				 >> 
					   *(lit(',')			[symbol_add(_val)]
						 >> tok.identifier	[at_c<1>(_val) = _1])
				 >> lit(';')			[symbol_add(_val)]
		;

		f_decl = tok.identifier				[at_c<1>(_val) = _1]
			   >> tok.identifier			[at_c<0>(_val) = _1]
			   >> lit('(')
			   > -(
					tok.identifier		    [push_back(at_c<2>(_val),_1)]
					>> -tok.identifier
					>> *(lit(',') > tok.identifier [push_back(at_c<2>(_val),_1)]
								  > -tok.identifier)
				  )
			   > lit(')')				[function_decl_add(_val)]
			   > -lit(';')
			   
	    ;

		/* 
		 * XXX
		 * structure_decl and v_decl are more or less the same, so maybe there
		 * is a way to share the code between them. For moment, it works as it
		 */
		structure_decl = tok.identifier	    [at_c<0>(_val) = _1]
					  >> lit('=')
					  >> tok.struct_
					  >> lit('{')
						>> *(tok.identifier	[at_c<0>(_a) = _1]
						>> tok.identifier   [at_c<1>(_a) = _1]
						>>
							*(lit(',')	[push_back(at_c<1>(_val), _a)]
							>> tok.identifier [at_c<1>(_a) = _1])
						>> lit(';')		[push_back(at_c<1>(_val), _a)])
					  >> lit('}')		[struct_decl_add(_val)]
					  >> -lit(';')		  
					  ;

		new_type_decl = tok.identifier		[at_c<0>(_val) = _1]
					 >> lit('=')
					 >> tok.newtype_
					 >> tok.identifier		[at_c<1>(_val) = _1]
					 >> lit(';')		[newtype_decl_adder(_val)]
					 ;

		type_decl =
					structure_decl
				   |new_type_decl
				   ;

		statement_ = 
					v_decl
				   |f_decl
				   |type_decl
				   ;
		statement_list = +statement_;
	
		v_decl.name("var decl");
		f_decl.name("function declation");
		type_decl.name("type declaration");
		structure_decl.name("struct declaration");
		new_type_decl.name("newtype declaration");
		statement_.name("statement");
		statement_list.name("statement-list");

		qi::on_error<qi::fail> (statement_list, error_handler(_4, _3, _2));
	};

	qi::rule<Iterator, white_space_> statement_, statement_list, type_decl; 
	qi::rule<Iterator, var_decl(), white_space_> v_decl;
	qi::rule<Iterator, function_decl(), white_space_> f_decl;
	qi::rule<Iterator, struct_decl(), qi::locals<var_decl>, white_space_> structure_decl;
	qi::rule<Iterator, newtype_decl(), white_space_> new_type_decl;

	function<symbol_adder> symbol_add;
	function<function_adder> function_decl_add;
	function<struct_adder> struct_decl_add;
	function<newtype_adder> newtype_decl_adder;
};



#if 0
template <typename Iterator>
struct HyperGrammar : qi::grammar<Iterator, white_space<Iterator> >
{
	    HyperGrammar() : HyperGrammar::base_type(expression)
        {

            expression =
                term
                >> *(   ('+' >> term)
                    |   ('-' >> term)
                    )
                ;

            term =
                factor
                >> *(   ('*' >> factor)
                    |   ('/' >> factor)
                    )
                ;

            factor =
                qi::uint_
                |   '(' >> expression >> ')'
                |   ('-' >> factor)
                |   ('+' >> factor)
                ;
        }

		qi::rule<Iterator, white_space<Iterator> > expression, term, factor;
};
#endif

bool parser::parse(const std::string & expr) 
{
    // iterator type used to expose the underlying input stream
    typedef std::string::const_iterator base_iterator_type;

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
    typedef lex::lexertl::token<
        base_iterator_type, boost::mpl::vector<std::string> 
    > token_type;

    // Here we use the lexertl based lexer engine.
    typedef lex::lexertl::lexer<token_type> lexer_type;

    // This is the token definition type (derived from the given lexer type).
    typedef hyper_lexer<lexer_type> hyper_lexer;

    // this is the iterator type exposed by the lexer 
    typedef hyper_lexer::iterator_type iterator_type;

    // this is the type of the grammar to parse
    typedef expression<iterator_type, hyper_lexer::lexer_def> hyper_grammar;

	hyper_lexer our_lexer;
	hyper_grammar g(our_lexer, sList, fList);

	base_iterator_type it = expr.begin();
	iterator_type iter = our_lexer.begin(it, expr.end());
	iterator_type end = our_lexer.end();
    bool r = phrase_parse(iter, end, g, qi::in_state("WS")[our_lexer.self]);

	if (r && iter == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
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
