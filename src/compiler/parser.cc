
#include <compiler/parser.hh>

#include <boost/spirit/include/qi.hpp>
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

template <typename Iterator>
struct white_space : qi::grammar<Iterator>
{
    white_space() : white_space::base_type(start)
    {
        using boost::spirit::ascii::char_;

        start =
                ascii::space                         // tab/space/cr/lf
            |   "/*" >> *(char_ - "*/") >> "*/"     // C-style comments
			|   "//" >> *(char_ - '\n') >> '\n'     // C++ comment
            ;
    }

	qi::rule<Iterator> start;
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


template <typename Iterator>
struct expression : qi::grammar<Iterator, white_space<Iterator> >
{
    typedef white_space<Iterator> white_space_;

    expression(symbolList &s, functionDefList& f) :
								expression::base_type(statement_list, "statement_list"),
								symbol_add(s),
								function_decl_add(f)
	{
	    using qi::lit;
        using qi::lexeme;
        using ascii::char_;
		using ascii::alpha;
        using namespace qi::labels;

		using phoenix::at_c;
        using phoenix::push_back;

		identifier = lexeme[+alpha		[_val += _1]];

		v_decl   = identifier			[at_c<0>(_val) = _1]
				 >> identifier			[at_c<1>(_val) = _1]
				 >> 
					   *(lit(',')			[symbol_add(_val)]
						 >> identifier	[at_c<1>(_val) = _1])
				 >> lit(';')			[symbol_add(_val)]
		;

		f_decl = identifier				[at_c<1>(_val) = _1]
			   >> identifier			[at_c<0>(_val) = _1]
			   >> lit('(')
			   > -(
					identifier		    [push_back(at_c<2>(_val),_1)]
					>> *(lit(',') > identifier [push_back(at_c<2>(_val),_1)])
				  )
			   > lit(')')				[function_decl_add(_val)]
			   > -lit(';')
			   
	    ;

		statement_ = 
					v_decl
				   |f_decl
				   ;
		statement_list = +statement_;
	
		identifier.name("identifier");
		v_decl.name("var decl");
		f_decl.name("function declation");
		statement_.name("statement");
		statement_list.name("statement-list");

		qi::on_error<qi::fail> (statement_list, error_handler(_4, _3, _2));
	};

	qi::rule<Iterator, white_space_> statement_, statement_list;
	qi::rule<Iterator, var_decl(), white_space_> v_decl;
	qi::rule<Iterator, function_decl(), white_space_> f_decl;
	qi::rule<Iterator, std::string(), white_space_> identifier;

	function<symbol_adder> symbol_add;
	function<function_adder> function_decl_add;
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
    typedef white_space<std::string::const_iterator> white_space;
	white_space ws;

	expression<std::string::const_iterator> g(sList, fList);

    std::string::const_iterator iter = expr.begin();
    std::string::const_iterator end = expr.end();
    bool r = phrase_parse(iter, end, g, ws);

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
