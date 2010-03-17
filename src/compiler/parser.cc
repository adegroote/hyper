#include <fstream>
#include <iostream>
#include <sstream>

#include <compiler/parser.hh>
#include <compiler/ability_parser.hh>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_algorithm.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
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
		struct_ = "struct";
		newtype_ = "newtype";
		ability_ = "ability";
		context_ = "context";
		tasks_ = "tasks";
		export_ = "export";
		import_ = "import";

		/* identifier must be the last if you want to not match keyword */
        this->self = lex::token_def<>('(') | ')' | '{' | '}' | '=' | ';' | ',' ;
		this->self += struct_ | newtype_ | ability_ | context_ | tasks_ | export_ | import_;
		this->self += identifier | scoped_identifier;
		this->self += constant_string;

        // define the whitespace to ignore (spaces, tabs, newlines and C-style 
        // comments)
        this->self("WS")
            =   lex::token_def<>("[ \\t\\n]+") 
            |   "\\/\\*[^*]*\\*+([^/*][^*]*\\*+)*\\/"
            ;
	};

    lex::token_def<> struct_, newtype_, ability_, context_, tasks_, export_, import_;
    lex::token_def<std::string> identifier, scoped_identifier;
	lex::token_def<std::string> constant_string;
};

BOOST_FUSION_ADAPT_STRUCT(
    symbol_decl,
    (std::string, typeName)
	(std::string, name)
);

BOOST_FUSION_ADAPT_STRUCT(
	symbol_decl_list,
	(std::vector<symbol_decl>, l)
);

BOOST_FUSION_ADAPT_STRUCT(
    function_decl,
    (std::string, fName)
	(std::string, returnName)
	(std::vector < std::string>, argsName)
);

BOOST_FUSION_ADAPT_STRUCT(
	function_decl_list,
	(std::vector<function_decl> , l)
);

BOOST_FUSION_ADAPT_STRUCT(
	struct_decl,
	(std::string, name)
	(symbol_decl_list, vars)
);

BOOST_FUSION_ADAPT_STRUCT(
	newtype_decl,
	(std::string, newname)
	(std::string, oldname)
);

BOOST_FUSION_ADAPT_STRUCT(
	type_decl_list,
	(std::vector < type_decl >, l)
);

BOOST_FUSION_ADAPT_STRUCT(
	programming_decl,
	(type_decl_list, types)
	(function_decl_list, funcs)
);

BOOST_FUSION_ADAPT_STRUCT(
	ability_blocks_decl,
	(symbol_decl_list, controlables)    // 0
	(symbol_decl_list, readables)       // 1
	(symbol_decl_list, privates)        // 2
	(programming_decl, env)				// 3
);

BOOST_FUSION_ADAPT_STRUCT(
	ability_decl,
	(std::string, name)				    // 0
	(ability_blocks_decl, blocks)		// 1
);	

struct ability_add_adaptator {
	universe &u;

	template <typename>
    struct result { typedef bool type; };

	ability_add_adaptator(universe & u_) : u(u_) {};

	bool operator()(const ability_decl& decl) const
	{
		return u.add(decl);
	}
};

struct parse_import {

	parser &p;

	template <typename>
	struct result { typedef bool type; };

	parse_import(parser & p_) : p(p_) {};

	bool operator()(const std::string& filename) const
	{
		// remove first and last "
		std::string f = filename.substr(1, filename.size() - 2);
		return p.parse_ability_file(f);
	}
};

template <typename Iterator, typename Lexer>
struct  grammar_ability: qi::grammar<Iterator, qi::in_state_skipper<Lexer> >
{
    typedef qi::in_state_skipper<Lexer> white_space_;

	template <typename TokenDef>
    grammar_ability(const TokenDef& tok, universe& u_, parser &p_) : 
		grammar_ability::base_type(statement, "ability"), 
		ability_adder(u_), import_adder(p_)
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
		
		statement =
				  import_list
				  >> ability_			   [ability_adder(_1)]
				  ;

		ability_ = 
				  import_list
				  >> tok.identifier			[swap(at_c<0>(_val), _1)] 
				  >> lit('=')
				  >> tok.ability_ 
				  >> lit('{')
				  >> ability_description	[swap(at_c<1>(_val), _1)]
				  >> lit('}')
				  >> -lit(';')
				  ;

		ability_description = 
				        block_context		[swap(_val, _1)]
					>> -block_tasks
				    >> -block_definition	[swap(at_c<3>(_val), _1)]
					;	

		block_context =
				  tok.context_
				  >> lit('=')
				  >> lit('{')
				  >> block_variable		[swap(at_c<0>(_val), _1)]
				  >> block_variable		[swap(at_c<1>(_val), _1)]
				  >> block_variable		[swap(at_c<2>(_val), _1)]
				  >> lit('}')
				  ;

		block_variable =
				  lit('{')
				  >> v_decl_list		 [swap(_val, _1)]
				  >> lit('}')
				  ;

		block_tasks = 
				  tok.tasks_
				  >> lit('=')
				  >> lit('{')
				  >> lit('}')
				  ;

		block_definition =
				 tok.export_
				 >> lit('=')
				 >> lit('{')
				 >> block_type_decl			[swap(at_c<0>(_val), _1)]
				 >> block_function_decl     [swap(at_c<1>(_val), _1)]
				 >> lit('}')
				 ;

		block_type_decl =
				lit('{')
				>> type_decl_list_		[swap(_val, _1)]
				>> lit('}')
				;

		block_function_decl =
				lit('{')
				>> f_decl_list		    [swap(_val, _1)]
				>> lit('}')
				;

		import_list =
				*(import				[import_adder(_1)])
				;

		import =	tok.import_ 
					>> tok.constant_string	[swap(_val, _1)]
					>> -lit(';')
					;

		v_decl_list = (*v_decl			    [insert(at_c<0>(_val), end(at_c<0>(_val)), 
											  begin(at_c<0>(_1)), end(at_c<0>(_1)))])
				   ;

		v_decl   =  (tok.identifier|tok.scoped_identifier)	
											[at_c<0>(_a) = _1]
				 >> tok.identifier			[at_c<1>(_a) = _1]
				 >> 
					   *(lit(',')			[push_back(at_c<0>(_val), _a)]
						 >> tok.identifier	[at_c<1>(_a) = _1])
				 >> lit(';')				[push_back(at_c<0>(_val), _a)]
		;

		f_decl_list =
				(*f_decl							  [push_back(at_c<0>(_val), _1)])
				;

		f_decl = (tok.identifier|tok.scoped_identifier)								[at_c<1>(_val) = _1]
			   >> tok.identifier													[at_c<0>(_val) = _1]
			   >> lit('(')
			   > -(
					(tok.identifier|tok.scoped_identifier)							[push_back(at_c<2>(_val),_1)]
					>> -tok.identifier
					>> *(lit(',') > (tok.identifier|tok.scoped_identifier)			[push_back(at_c<2>(_val),_1)]
								  > -tok.identifier)
				  )
			   > lit(')')				
			   > -lit(';')
			   
	    ;

		type_decl_ = 
					structure_decl						[_val =  _1]
				   |new_type_decl						[_val =  _1]
				   ;

		type_decl_list_ = 
					 (*type_decl_					    [push_back(at_c<0>(_val), _1)])
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
					  >> v_decl_list		[swap(at_c<1>(_val) , _1)]
					  >> lit('}')			
					  >> -lit(';')		  
					  ;

		new_type_decl = tok.identifier		[at_c<0>(_val) = _1]
					 >> lit('=')
					 >> tok.newtype_
					 >> (tok.identifier|tok.scoped_identifier)	[at_c<1>(_val) = _1]
					 >> lit(';')		
					 ;


		ability_.name("ability declaration");
		ability_description.name("ability description");
		block_context.name("context block");
		block_tasks.name("block_tasks");
		block_definition.name("block_definition");
		block_variable.name("block_variable");
		block_type_decl.name("block_type_decl");
		block_function_decl.name("block_function_decl");
		import_list.name("import_list");
		import.name("import");
		v_decl.name("var decl");
		v_decl_list.name("var declaration list");
		f_decl.name("function declation");
		f_decl_list.name("function declaration list");
		type_decl_.name("type declaration");
		type_decl_list_.name("type declaration list");
		structure_decl.name("struct declaration");
		new_type_decl.name("newtype declaration");

		qi::on_error<qi::fail> (ability_, error_handler(_4, _3, _2));

#ifdef HYPER_DEBUG_RULES
		debug(ability_);
		debug(ability_description);
		debug(block_context);
		debug(block_tasks);
		debug(block_definition);
		debug(block_variable);
		debug(block_type_decl);
		debug(block_function_decl);
		debug(import_list);
		debug(import);
		debug(v_decl);
		debug(v_decl_list);
		debug(f_decl);
		debug(f_decl_list);
		debug(type_decl_);
		debug(type_decl_list_);
		debug(structure_decl);
		debug(new_type_decl);
#endif
	};

	qi::rule<Iterator, white_space_> statement;
	qi::rule<Iterator, white_space_> block_tasks;
	qi::rule<Iterator, white_space_> import_list;
	qi::rule<Iterator, std::string(), white_space_> import;
	qi::rule<Iterator, ability_decl(), white_space_> ability_;
	qi::rule<Iterator, programming_decl(), white_space_> block_definition; 
	qi::rule<Iterator, ability_blocks_decl(), white_space_> block_context, ability_description;
	qi::rule<Iterator, type_decl(), white_space_>  type_decl_; 
	qi::rule<Iterator, type_decl_list(), white_space_> type_decl_list_, block_type_decl;
	qi::rule<Iterator, symbol_decl_list(), qi::locals<symbol_decl>, white_space_> v_decl;
	qi::rule<Iterator, symbol_decl_list(), white_space_> v_decl_list, block_variable;
	qi::rule<Iterator, function_decl(), white_space_> f_decl;
	qi::rule<Iterator, function_decl_list(), white_space_> f_decl_list, block_function_decl;
	qi::rule<Iterator, struct_decl(), qi::locals<symbol_decl>, white_space_> structure_decl;
	qi::rule<Iterator, newtype_decl(), white_space_> new_type_decl;

	function<ability_add_adaptator> ability_adder;
	function<parse_import> import_adder;
};

std::string 
read_from_file(const std::string& filename)
{
    std::ifstream instream(filename.c_str());
    if (!instream.is_open()) {
        std::cerr << "Couldn't open file: " << filename << std::endl;
        exit(-1);
    }
    instream.unsetf(std::ios::skipws);      // No white space skipping!
    return std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
                       std::istreambuf_iterator<char>());
}


bool parser::parse_ability_file(const std::string & filename) 
{
	std::cout << "parsing " << filename << std::endl;
	// Avoid to parse twice or more the same files
	using namespace boost::filesystem;

	path base(filename);
	path full = complete(base);

	std::vector<path>::const_iterator it_parsed;
	it_parsed = std::find(parsed_files.begin(), parsed_files.end(), full);
	if (it_parsed != parsed_files.end()) {
		std::cout << "already parsed " << filename << " : skip it !!! " << std::endl;
		return true; // already parsed
	}

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

    typedef std::string::iterator base_iterator_type;

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
    typedef grammar_ability<iterator_type, hyper_lexer::lexer_def> hyper_ability;

	hyper_lexer our_lexer;
	hyper_ability g(our_lexer, u, *this);

	std::string expr = read_from_file(filename);
	base_iterator_type it = expr.begin();
	iterator_type iter = our_lexer.begin(it, expr.end());
	iterator_type end = our_lexer.end();
    bool r = phrase_parse(iter, end, g, qi::in_state("WS")[our_lexer.self]);

	parsed_files.push_back(full);

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
