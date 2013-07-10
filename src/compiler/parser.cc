#include <iostream>

#include <compiler/ability_parser.hh>
#include <compiler/base_parser.hh>
#include <compiler/import_parser.hh>
#include <compiler/parser.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>


//#define HYPER_DEBUG_RULES
//
using boost::phoenix::function;
using boost::phoenix::ref;
using boost::phoenix::size;

using namespace hyper::compiler;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;

BOOST_FUSION_ADAPT_STRUCT(
    symbol_decl,
    (std::string, typeName)
	(std::string, name)
	(expression_ast, initializer)
)

BOOST_FUSION_ADAPT_STRUCT(
	symbol_decl_list,
	(std::vector<symbol_decl>, l)
)

BOOST_FUSION_ADAPT_STRUCT(
    function_decl,
    (std::string, fName)
	(std::string, returnName)
	(std::vector < std::string>, argsName)
	(boost::optional<std::string>, tag)
)

BOOST_FUSION_ADAPT_STRUCT(
	function_decl_list,
	(std::vector<function_decl> , l)
)

BOOST_FUSION_ADAPT_STRUCT(
	struct_decl,
	(std::string, name)
	(symbol_decl_list, vars)
)

BOOST_FUSION_ADAPT_STRUCT(
	newtype_decl,
	(std::string, newname)
	(std::string, oldname)
)

BOOST_FUSION_ADAPT_STRUCT(
	opaque_decl,
	(std::string, name)
)

BOOST_FUSION_ADAPT_STRUCT(
	type_decl_list,
	(std::vector < type_decl >, l)
)

BOOST_FUSION_ADAPT_STRUCT(
	rule_decl,
	(std::string, name)
	(std::vector<expression_ast>, premises)
	(std::vector<expression_ast>, conclusions)
)

BOOST_FUSION_ADAPT_STRUCT(
	rule_decl_list,
	(std::vector<rule_decl>, l)
)

BOOST_FUSION_ADAPT_STRUCT(
	programming_decl,
	(type_decl_list, types)
	(function_decl_list, funcs)
	(rule_decl_list, rules)
)

BOOST_FUSION_ADAPT_STRUCT(
	ability_blocks_decl,
	(symbol_decl_list, controlables)    // 0
	(symbol_decl_list, readables)       // 1
	(symbol_decl_list, privates)        // 2
	(programming_decl, env)				// 3
)

BOOST_FUSION_ADAPT_STRUCT(
	ability_decl,
	(std::string, name)				    // 0
	(ability_blocks_decl, blocks)		// 1
)	

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

template <typename Iterator>
struct initializer_grammar: qi::grammar<Iterator, expression_ast(), white_space<Iterator> >
{
	typedef white_space<Iterator> white_space_;

	initializer_grammar() : initializer_grammar::base_type(start)
	{
		using namespace qi::labels;
		using phoenix::at_c;

		start =  task_call						
			  |	atom							
			  ;

		task_call = identifier [at_c<0>(_val) = _1]
					>> qi::lit('(')
					> qi::lit(')')
			;

		qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
	}

	qi::rule<Iterator, expression_ast(), white_space_> start;
	qi::rule<Iterator, function_call(), white_space_> task_call;
	identifier_grammar<Iterator> identifier;
	atom_grammar<Iterator> atom;
};

template <typename Iterator>
struct  grammar_ability: qi::grammar<Iterator, white_space<Iterator> >
{
    typedef white_space<Iterator> white_space_;

    grammar_ability(universe& u_, hyper::compiler::parser &p_) : 
		grammar_ability::base_type(statement, "ability"), 
		ability_adder(u_), import_list(p_)
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
				  identifier
				  > lit('=')
				  > lit("ability")
				  > lit('{')
				  > ability_description
				  > lit('}')
				  > -lit(';')
				  ;

		ability_description = 
				        block_context		[swap(_val, _1)]
					> -block_tasks
				    > -block_definition	[swap(at_c<3>(_val), _1)]
					;	

		block_context =
				  lit("context")
				  > lit('=')
				  > lit('{')
				  > block_variable
				  > block_variable
				  > block_variable
				  > lit('}')
				  ;

		block_variable =
				  lit('{')
				  > v_decl_list		 [swap(_val, _1)]
				  > lit('}')
				  ;

		block_tasks = 
				  lit("tasks")
				  > lit('=')
				  > lit('{')
				  > lit('}')
				  ;

		block_definition =
				 lit("export")
				 > lit('=')
				 > lit('{')
				 > -block_type_decl
				 > -block_function_decl
				 > -block_rule_decl
				 > lit('}')
				 ;

		block_type_decl =
				lit('{')
				> type_decl_list_
				> lit('}')
				;

		block_function_decl =
				lit('{')
				> f_decl_list
				> lit('}')
				;

		block_rule_decl = 
				lit('{')
				> rule_decl_list_
				> lit('}')
				;

		v_decl_list = (*v_decl			    [insert(at_c<0>(_val), end(at_c<0>(_val)), 
											  begin(at_c<0>(_1)), end(at_c<0>(_1)))])
				   ;

		v_decl   =  scoped_identifier		[at_c<0>(_a) = _1]
				 > identifier				[at_c<1>(_a) = _1]
				 >> -( lit('=') > initializer [at_c<2>(_a) = _1])
				 > 
					   *(lit(',')			[push_back(at_c<0>(_val), _a)]
						 > identifier		[at_c<1>(_a) = _1]
						 > -(lit('=') > initializer [at_c<2>(_a) = _1])
						)
				 > lit(';')				[push_back(at_c<0>(_val), _a)]
		;

		f_decl_list =(*f_decl);

		tag_decl = lit('@') > identifier;

		f_decl =  -(tag_decl								    [at_c<3>(_val) = _1])
			   >> scoped_identifier								[at_c<1>(_val) = _1]
			   > identifier									[at_c<0>(_val) = _1]
			   > lit('(')
			   > -((
					scoped_identifier							[push_back(at_c<2>(_val),_1)]
					> - identifier
				   ) % ','
				  )
			   > lit(')')
			   > -lit(';')
			   
	    ;

		type_decl_ = structure_decl
				   |new_type_decl
				   |opaque_decl_
				   ;

		type_decl_list_ = (*type_decl_);


		/* 
		 * XXX
		 * structure_decl and v_decl are more or less the same, so maybe there
		 * is a way to share the code between them. For moment, it works as it
		 */
		structure_decl = identifier
					  >> lit('=')
					  >> lit("struct")
					  > lit('{')
					  > v_decl_list
					  > lit('}')
					  > -lit(';')
					  ;

		new_type_decl = identifier			
					 >> lit('=')
					 >> lit("newtype")
					 > scoped_identifier
					 > -lit(';')		
					 ;

		opaque_decl_ = identifier 
					 >> lit('=')
					 >> lit ("opaquetype")
					 > -lit(';')
					 ;

		rule_decl_ = identifier				 
					 >> lit('=')
					 >> -( expression	% ',')		 
					 >> lit("|-")
					 >> -( expression	% ',')		 
					 > -lit(';')
					 ;

		rule_decl_list_ = (*rule_decl_);


		ability_.name("ability declaration");
		ability_description.name("ability description");
		block_context.name("context block");
		block_tasks.name("block_tasks");
		block_definition.name("block_definition");
		block_variable.name("block_variable");
		block_type_decl.name("block_type_decl");
		block_function_decl.name("block_function_decl");
		v_decl.name("var decl");
		v_decl_list.name("var declaration list");
		f_decl.name("function declation");
		f_decl_list.name("function declaration list");
		type_decl_.name("type declaration");
		type_decl_list_.name("type declaration list");
		structure_decl.name("struct declaration");
		new_type_decl.name("newtype declaration");
		opaque_decl_.name("opaque type declaration");
		tag_decl.name("tag declaration");
		rule_decl_.name("rule declaration");
		rule_decl_list_.name("rule list declaration");
		block_rule_decl.name("block rule declartion");

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
		debug(v_decl);
		debug(v_decl_list);
		debug(f_decl);
		debug(f_decl_list);
		debug(type_decl_);
		debug(type_decl_list_);
		debug(structure_decl);
		debug(new_type_decl);
		debug(opaque_decl_);
		debug(tag_decl);
		debug(rule_decl_);
		debug(rule_decl_list_);
		debug(block_rule_decl);
#endif
	}

	qi::rule<Iterator, white_space_> statement;
	qi::rule<Iterator, white_space_> block_tasks;
	qi::rule<Iterator, ability_decl(), white_space_> ability_;
	qi::rule<Iterator, programming_decl(), white_space_> block_definition; 
	qi::rule<Iterator, ability_blocks_decl(), white_space_> block_context, ability_description;
	qi::rule<Iterator, type_decl(), white_space_>  type_decl_; 
	qi::rule<Iterator, type_decl_list(), white_space_> block_type_decl;
	qi::rule<Iterator, std::vector<type_decl>(), white_space_> type_decl_list_;
	qi::rule<Iterator, symbol_decl_list(), qi::locals<symbol_decl>, white_space_> v_decl;
	qi::rule<Iterator, symbol_decl_list(), white_space_> v_decl_list, block_variable;
	qi::rule<Iterator, function_decl(), white_space_> f_decl;
	qi::rule<Iterator, function_decl_list(), white_space_> block_function_decl;
	qi::rule<Iterator, std::vector<function_decl>(), white_space_> f_decl_list;
	qi::rule<Iterator, struct_decl(), qi::locals<symbol_decl>, white_space_> structure_decl;
	qi::rule<Iterator, newtype_decl(), white_space_> new_type_decl;
	qi::rule<Iterator, opaque_decl(), white_space_> opaque_decl_;
	qi::rule<Iterator, std::string(), white_space_> tag_decl;
	qi::rule<Iterator, rule_decl(), white_space_> rule_decl_;
	qi::rule<Iterator, std::vector<rule_decl>(), white_space_> rule_decl_list_;
	qi::rule<Iterator, rule_decl_list(), white_space_> block_rule_decl;

	function<ability_add_adaptator> ability_adder;
	identifier_grammar<Iterator> identifier;
	scoped_identifier_grammar<Iterator> scoped_identifier;
	initializer_grammar<Iterator> initializer;
	import_grammar<Iterator> import_list;
	grammar_expression<Iterator> expression;
};



bool parser::parse_ability_file(const std::string & filename) 
{
	if (u.is_verbose())
		std::cout << "parsing " << filename << std::endl;
	// Avoid to parse twice or more the same files
	using namespace boost::filesystem;

	path base(filename);
	path full = complete(base);

	std::vector<path>::const_iterator it_parsed;
	it_parsed = std::find(parsed_files.begin(), parsed_files.end(), full);
	if (it_parsed != parsed_files.end()) {
		if (u.is_verbose())
			std::cout << "already parsed " << filename << " : skip it !!! " << std::endl;
		return true; // already parsed
	}

    // this is the type of the grammar to parse
    typedef grammar_ability<std::string::const_iterator> hyper_ability;

	hyper_ability g(u, *this);

	std::string expr = read_from_file(filename);
	bool r;
	try {
		r = parse(g, expr);
	} catch (hyper::compiler::import_exception& e) {
		std::cerr << e.what() << std::endl;
		return false;
	}

	if (r) 
		parsed_files.push_back(full);
	return r;
}
