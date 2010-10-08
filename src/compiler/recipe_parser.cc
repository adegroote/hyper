
#include <compiler/parser.hh>
#include <compiler/cond_block_parser.hh>
#include <compiler/recipe_parser.hh>
#include <compiler/utils.hh>

using namespace hyper::compiler;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;

//#define HYPER_DEBUG_RULES


std::ostream& hyper::compiler::operator<< (std::ostream& os, const body_block_decl& d)
{
	std::copy(d.list.begin(), d.list.end(), std::ostream_iterator<recipe_expression>( os ));
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const recipe_decl& r)
{
	os << r.name;
	os << r.conds;
	os << r.body;
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const recipe_decl_list & l)
{
	std::copy(l.recipes.begin(), l.recipes.end(), std::ostream_iterator<recipe_decl>( os ));
	return os;
}

BOOST_FUSION_ADAPT_STRUCT(
	body_block_decl,
	(std::vector<recipe_expression>, list)
)

BOOST_FUSION_ADAPT_STRUCT(
	recipe_decl,
	(std::string, name)
	(cond_block_decl, conds)
	(body_block_decl, body)
)

BOOST_FUSION_ADAPT_STRUCT(
	recipe_decl_list,
	(std::vector<recipe_decl>, recipes)
)

BOOST_FUSION_ADAPT_STRUCT(
	let_decl,
	(std::string, identifier)
	(recipe_expression, bounded)
)

BOOST_FUSION_ADAPT_STRUCT(
	abort_decl,
	(std::string, identifier)
)

phoenix::function<error_handler_> const error_handler = error_handler_();

template <typename Iterator>
struct body_block_grammar :
	qi::grammar<Iterator, body_block_decl(), white_space<Iterator> >
{
	typedef white_space<Iterator> white_space_;

	body_block_grammar() :
		body_block_grammar::base_type(start)
	{
		using qi::lit;
        using namespace qi::labels;

		using phoenix::at_c;
		using phoenix::swap;

		start = lit("body")
			>> lit("=")
			>> lit('{')
			>> body_decl_list	[swap(at_c<0>(_val), _1)]
			>> lit('}')
			>> -lit(';')
			;

		body_decl_list = (*body_decl	);

		body_decl = (	let_decl_	
					|	make_decl_
					|   ensure_decl_
					|   wait_decl_
					|	abort_decl_
					|	expression
					)
					>> -lit(';')
					;

		/* 
		 * XXX not the right grammar here, it is probably only right for ensure
		 * and expression
		 */
		let_decl_ = lit("let")
				 >> identifier		[at_c<0>(_val) = _1]
				 >> body_decl		[at_c<1>(_val) = _1]
				 ;

		make_decl_ = lit("make")
				  >> lit("(")
				  >> expression
				  >> lit(")")
				  ;

		ensure_decl_ = lit("ensure")
				  >> lit("(")
				  >> expression
				  >> lit(")")
				  ;

		abort_decl_ = lit("abort")
				   >> identifier	[at_c<0>(_val) = _1]
				   ;


		wait_decl_ = lit("wait")
				  >> lit("(")
				  >> expression
				  >> lit(")")
				  ;

		start.name("body block");
		body_decl_list.name("recipe expression list");
		body_decl.name("recipe expression");
		let_decl_.name("let declaration");
		abort_decl_.name("abort declaration");
		make_decl_.name("make declaration");
		ensure_decl_.name("ensure declaration");
		wait_decl_.name("wait declaration");

#ifdef HYPER_DEBUG_RULES
		debug(start);
		debug(body_decl_list);
		debug(body_decl);
		debug(let_decl_);
		debug(abort_decl_);
		debug(make_decl_);
		debug(ensure_decl_);
		debug(wait_decl_);
#endif
	}

	qi::rule<Iterator, body_block_decl(), white_space_> start;
	qi::rule<Iterator, std::vector<recipe_expression>(), white_space_> body_decl_list;
	qi::rule<Iterator, recipe_expression(), white_space_> body_decl;
	qi::rule<Iterator, let_decl(), white_space_> let_decl_;
	qi::rule<Iterator, abort_decl(), white_space_> abort_decl_;
	qi::rule<Iterator, recipe_op<MAKE>(), white_space_> make_decl_;
	qi::rule<Iterator, recipe_op<ENSURE>(), white_space_> ensure_decl_;
	qi::rule<Iterator, recipe_op<WAIT>(), white_space_> wait_decl_;

	grammar_expression<Iterator> expression;
	identifier_grammar<Iterator> identifier;
};

template <typename Iterator>
struct  grammar_recipe : 
	qi::grammar<Iterator, recipe_decl_list(), white_space<Iterator> >
{
    typedef white_space<Iterator> white_space_;

    grammar_recipe() : 
		grammar_recipe::base_type(start)
	{
	    using qi::lit;
        using qi::lexeme;
        using ascii::char_;
        using namespace qi::labels;

		using phoenix::at_c;
        using phoenix::push_back;
		using phoenix::swap;

		start = 
			 (*recipe			[push_back(at_c<0>(_val), _1)])
			 ;

		recipe = scoped_identifier  [swap(at_c<0>(_val), _1)]
			 >> lit('=')
			 >> lit("recipe")
			 >> lit("{")
			 >> cond_block			[swap(at_c<1>(_val), _1)]
			 >> body_block			[swap(at_c<2>(_val), _1)]
			 >> lit('}')
			 >> -lit(';')
			 ;

		start.name("recipe list");
		recipe.name("recipe");

#ifdef HYPER_DEBUG_RULES
		debug(start);
		debug(recipe);
#endif

		qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
	}

	qi::rule<Iterator, recipe_decl_list(), white_space_> start;
	qi::rule<Iterator, recipe_decl(), white_space_> recipe;

	scoped_identifier_grammar<Iterator> scoped_identifier;
	cond_block_grammar<Iterator> cond_block;
	body_block_grammar<Iterator> body_block;
};


bool parser::parse_recipe(const std::string& expr)
{
	recipe_decl_list result;
    bool r = parse(grammar_recipe<std::string::const_iterator>(), expr, result);
	std::cout << result << std::endl;
	return r;
}

bool parser::parse_recipe_file(const std::string& fileName)
{
	std::string content = read_from_file(fileName);
	return parse_task(content);
}
