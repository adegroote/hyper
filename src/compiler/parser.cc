#include <fstream>
#include <iostream>
#include <sstream>

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

struct symbol_adder {
	symbolList & s;

	template <typename>
    struct result { typedef void type; };

	symbol_adder(symbolList & s_) : s(s_) {};

	void print_diagnostic(const symbolList::add_result & res, const symbol_decl& decl) const
	{
		if ( res.first == false ) {
			switch ( res.second ) {
				case symbolList::alreadyExist: 
					std::cerr << "variable " << decl.name << " already defined" << std::endl;
					break;
				case symbolList::unknowType:
					std::cerr << "type " << decl.typeName << " used to declare " << decl.name;
					std::cerr << " is undefined" << std::endl;
					break;
				case symbolList::noError:
				default:
					;
			}
		} else {
			std::cout << "Successfully add var " << decl.name << std::endl;
		}
	}

	void operator()(const symbol_decl& decl) const
	{
		symbolList::add_result p;
		p = s.add(decl);
		print_diagnostic(p, decl);
	};
};

struct function_decl {
	std::string fName;
	std::string returnName;
	std::vector < std::string > argsName;
};

std::ostream& operator << (std::ostream& os, const function_decl& decl)
{
	os << "function declaration of " << decl.fName << " ";
	return os;
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
	std::vector < symbol_decl > vars;
};

std::ostream& operator << (std::ostream& os, const struct_decl &decl)
{
	os << "struct declaration for " << decl.name << " ";
	return os;
}

BOOST_FUSION_ADAPT_STRUCT(
	struct_decl,
	(std::string, name)
	(std::vector < symbol_decl >, vars)
);

struct struct_adder {
	template<typename>
	struct result { typedef void type; };

	void operator() (const struct_decl& s) const 
	{
		std::cout << "struct " << s.name << " contains :" << std::endl;
		for (size_t i = 0; i < s.vars.size(); ++i)
			std::cout << "\t" << s.vars[i].typeName << " " << s.vars[i].name << std::endl;
	}
};

struct newtype_decl {
	std::string newname;
	std::string oldname;
};

std::ostream& operator << (std::ostream& os, const newtype_decl &decl)
{
	os << "newtype from " << decl.oldname << " to " << decl.newname << " ";
	return os;
}

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
	qi::rule<Iterator, symbol_decl(), white_space_> v_decl;
	qi::rule<Iterator, function_decl(), white_space_> f_decl;
	qi::rule<Iterator, struct_decl(), qi::locals<symbol_decl>, white_space_> structure_decl;
	qi::rule<Iterator, newtype_decl(), white_space_> new_type_decl;

	function<symbol_adder> symbol_add;
	function<function_adder> function_decl_add;
	function<struct_adder> struct_decl_add;
	function<newtype_adder> newtype_decl_adder;
};


template <typename Iterator, typename Lexer>
struct ability: qi::grammar<Iterator, qi::in_state_skipper<Lexer> >
{
    typedef qi::in_state_skipper<Lexer> white_space_;

	template <typename TokenDef>
    ability(const TokenDef& tok, symbolList& s, functionDefList& f) :
								   ability::base_type(ability_, "ability"),
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
		
		ability_ = 
				  import_list
				  >> tok.identifier 
				  >> lit('=')
				  >> tok.ability_ 
				  >> lit('{')
				  >> ability_description
				  >> lit('}')
				  >> -lit(';')
				  ;

		ability_description = 
				        block_context
					>> -block_tasks
				    >> -block_definition
					;	

		block_context =
				  tok.context_
				  >> lit('=')
				  >> lit('{')
				  >> block_variable
				  >> block_variable
				  >> block_variable
				  >> lit('}')
				  ;

		block_variable =
				  lit('{')
				  >> *(v_decl)
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
				 >> block_type_decl
				 >> block_function_decl
				 >> lit('}')
				 ;

		block_type_decl =
				lit('{')
				>> *(type_decl)
				>> lit('}')
				;

		block_function_decl =
				lit('{')
				>> *(f_decl)
				>> lit('}')
				;

		import_list =
				*import;

		import =	tok.import_ 
					>> tok.constant_string 
					>> -lit(';');


		v_decl   = (tok.identifier|tok.scoped_identifier)	
											[at_c<0>(_val) = _1]
				 >> tok.identifier			[at_c<1>(_val) = _1]
				 >> 
					   *(lit(',')			[symbol_add(_val)]
						 >> tok.identifier	[at_c<1>(_val) = _1])
				 >> lit(';')			[symbol_add(_val)]
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
			   > lit(')')				[function_decl_add(_val)]
			   > -lit(';')
			   
	    ;

		type_decl =
					structure_decl
				   |new_type_decl
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
						>> *((tok.identifier|tok.scoped_identifier)	[at_c<0>(_a) = _1]
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
					 >> (tok.identifier|tok.scoped_identifier)	[at_c<1>(_val) = _1]
					 >> lit(';')		[newtype_decl_adder(_val)]
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
		f_decl.name("function declation");
		type_decl.name("type declaration");
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
		debug(f_decl);
		debug(type_decl);
		debug(structure_decl);
		debug(new_type_decl);
#endif
	};

	qi::rule<Iterator, white_space_> ability_, ability_description;
	qi::rule<Iterator, white_space_> block_context, block_tasks, block_definition, block_variable;
	qi::rule<Iterator, white_space_> block_type_decl, block_function_decl;
	qi::rule<Iterator, white_space_> import_list, import;
	qi::rule<Iterator, white_space_>  type_decl; 
	qi::rule<Iterator, symbol_decl(), white_space_> v_decl;
	qi::rule<Iterator, function_decl(), white_space_> f_decl;
	qi::rule<Iterator, struct_decl(), qi::locals<symbol_decl>, white_space_> structure_decl;
	qi::rule<Iterator, newtype_decl(), white_space_> new_type_decl;

	function<symbol_adder> symbol_add;
	function<function_adder> function_decl_add;
	function<struct_adder> struct_decl_add;
	function<newtype_adder> newtype_decl_adder;
};



bool parser::parse(const std::string & expr) 
{
	// Stolen from boost/libs/spirit/examples/lex/example4.cpp
	//
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
    typedef ability<iterator_type, hyper_lexer::lexer_def> hyper_ability;

	hyper_lexer our_lexer;
	hyper_ability g(our_lexer, sList, fList);

	std::string expr = read_from_file(filename);
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
