#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include <compiler/expression_ast.hh>
#include <compiler/parser.hh>
#include <compiler/base_parser.hh>
#include <compiler/task_parser.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>

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

std::ostream& hyper::compiler::operator << (std::ostream& os, const cond_list_decl& l)
{
	std::copy(l.list.begin(), l.list.end(), std::ostream_iterator<expression_ast>( os ));
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const task_decl& t)
{
	os << t.name;
	os << "{" << t.pre << "}";
	os << "{" << t.post << "}";;
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const task_decl_list& l)
{
	std::copy(l.list.begin(), l.list.end(), std::ostream_iterator<task_decl>( os ));
	return os;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const task_decl_list_context& l)
{
	os << l.ability_name;
	os << "{" << l.list << "}";
	return os;
}

function<error_handler_> const error_handler = error_handler_();

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
		(std::string, fName)
		(std::vector<expression_ast>, args)
)

template <typename Iterator>
struct  grammar_expression : qi::grammar<Iterator, expression_ast(), white_space<Iterator> >
{
    typedef white_space<Iterator> white_space_;

    grammar_expression() : grammar_expression::base_type(expression, "expression")
	{
	    using qi::lit;
        using qi::lexeme;
        using ascii::char_;
        using namespace qi::labels;

		using phoenix::at_c;
		using phoenix::val;
		using phoenix::push_back;

		qi::real_parser<double, qi::strict_ureal_policies<double> > strict_double;

		expression =
			equality_expr.alias()
				;

		equality_expr =
			relational_expr								[_val = _1]
			>> *(		("==" > relational_expr      [bind(&expression_ast::binary<EQ>, _val, _1)])
					|   ("!=" > relational_expr     [bind(&expression_ast::binary<NEQ>, _val, _1)])
				)
			;

		relational_expr =
			logical_expr								[_val = _1]
			>> *(		("<=" > logical_expr        [bind(&expression_ast::binary<LTE>, _val, _1)])
					|   ('<' > logical_expr         [bind(&expression_ast::binary<LT>, _val, _1)])
					|   (">=" > logical_expr        [bind(&expression_ast::binary<GTE>, _val, _1)])
					|   ('>' > logical_expr         [bind(&expression_ast::binary<GT>, _val, _1)])
				)
			;

		logical_and_expr =
			additive_expr								[_val = _1]
			>> *(	   "&&" > additive_expr			[bind(&expression_ast::binary<AND>, _val, _1)])
			;

		logical_or_expr =
			logical_and_expr							[_val = _1]
			>> *(	"||" > logical_and_expr			[bind(&expression_ast::binary<OR>, _val, _1)])
			;

		logical_expr = logical_or_expr.alias()
			;
				
		additive_expr =
			multiplicative_expr						[_val = _1]
			>> *(		('+' > multiplicative_expr	[bind(&expression_ast::binary<ADD>, _val, _1)])
					|   ('-' > multiplicative_expr  [bind(&expression_ast::binary<SUB>, _val, _1)])
				)
			;

		multiplicative_expr =
			unary_expr							[_val = _1]
			>> *(		('*' > unary_expr		[bind(&expression_ast::binary<MUL>, _val, _1)])
					|   ('/' > unary_expr       [bind(&expression_ast::binary<DIV>, _val, _1)])
				)
			;


		unary_expr =
			primary_expr						[_val = _1]
			|   ('-' > primary_expr             [bind(&expression_ast::unary<NEG>, _val, _1)])
			|   ('+' > primary_expr				[_val = _1])
			;


		primary_expr =
					func_call						[_val = _1]
			    |	(node_							[_val = _1])
				|   ('(' > expression				[_val = _1] > ')')
			;


		node_ = 
				(
				cst_double			   
				| cst_int				   
				| cst_string			   
				| cst_bool
				| var_inst 			   
				)			   [_val = _1]
			 ;

		cst_int = qi::uint_		[at_c<0>(_val) = _1]
				;

		cst_double = strict_double [at_c<0>(_val) = _1]
				;

		cst_string = constant_string [at_c<0>(_val) = _1]
				;

		cst_bool = qi::bool_		    [at_c<0>(_val) = _1]
				 ;

		var_inst = scoped_identifier [_val = _1]
			;

		func_call = scoped_identifier [at_c<0>(_val) = _1]
				  >> lit('(')
				  >> -(
					   expression		[push_back(at_c<1>(_val), _1)]
					   >> *( ',' >
						     expression	[push_back(at_c<1>(_val), _1)])
					  )
				  >> lit(')')
				  ;

		expression.name("expression declaration");
		primary_expr.name("primary expr declaration");
		unary_expr.name("unary expr declaration");
		multiplicative_expr.name("multiplicative expr declaration");
		additive_expr.name("additive expr declaration");
		relational_expr.name("relational expr declaration");
		equality_expr.name("equality expr declaration");
		node_.name("node declaration");
		cst_int.name("const int");
		cst_double.name("const double");
		cst_string.name("const string");
		cst_bool.name("const bool");
		var_inst.name("var instance");
		func_call.name("function declaration instance");

		qi::on_error<qi::fail> (expression, error_handler(_4, _3, _2));
	}

	qi::rule<Iterator, expression_ast(), white_space_> expression, primary_expr, unary_expr;
	qi::rule<Iterator, expression_ast(), white_space_> multiplicative_expr, additive_expr;
	qi::rule<Iterator, expression_ast(), white_space_> logical_expr, logical_and_expr, logical_or_expr;
    qi::rule<Iterator, expression_ast(), white_space_> relational_expr, equality_expr;
	qi::rule<Iterator, expression_ast(), white_space_> node_;
	qi::rule<Iterator, Constant<int>(), white_space_> cst_int;
	qi::rule<Iterator, Constant<double>(), white_space_> cst_double;
	qi::rule<Iterator, Constant<bool>(), white_space_> cst_bool;
	qi::rule<Iterator, Constant<std::string>(), white_space_> cst_string;
	qi::rule<Iterator, std::string(), white_space_> var_inst;
	qi::rule<Iterator, function_call(), white_space_> func_call;

	const_string_grammer<Iterator> constant_string;
	scoped_identifier_grammar<Iterator> scoped_identifier;
};

BOOST_FUSION_ADAPT_STRUCT(
	cond_list_decl,
	(std::vector<expression_ast>, list)
)

BOOST_FUSION_ADAPT_STRUCT(
	task_decl,
	(std::string, name)
	(cond_list_decl, pre)
	(cond_list_decl, post)
)

BOOST_FUSION_ADAPT_STRUCT(
	task_decl_list,
	(std::vector<task_decl>, list)
)

BOOST_FUSION_ADAPT_STRUCT(
	task_decl_list_context,
	(std::string, ability_name)
	(task_decl_list, list)
)

template <typename Iterator>
struct  grammar_task : qi::grammar<Iterator, task_decl_list_context(), white_space<Iterator> >
{
    typedef white_space<Iterator> white_space_;

    grammar_task() : 
		grammar_task::base_type(task_decl_, "task_decl")
	{
	    using qi::lit;
        using qi::lexeme;
        using ascii::char_;
        using namespace qi::labels;

		using phoenix::at_c;
        using phoenix::push_back;
		using phoenix::swap;

		task_decl_ = 
				   lit("in")
				>> lit("context")
				>> scoped_identifier	 [swap(at_c<0>(_val), _1)]
				>> -lit(';')
				>> task_list			 [swap(at_c<1>(_val), _1)]
				;

		task_list = 
				(*task					[push_back(at_c<0>(_val), _1)])
				;

		task = scoped_identifier	[swap(at_c<0>(_val), _1)]
			 >> lit('=')
			 >> lit("task")
			 >> lit('{')
			 >> pre_cond				[swap(at_c<1>(_val), _1)]
			 >> post_cond				[swap(at_c<2>(_val), _1)]
			 >> lit('}')
			 >> -lit(';')
			 ;

		pre_cond = lit("pre")
				 >> cond				[swap(at_c<0>(_val), at_c<0>(_1))]
				 ;

		post_cond = lit("post")
				  >> cond				[swap(at_c<0>(_val), at_c<0>(_1))]
				  ;

		cond = lit('=')
			 >> lit('{')
			 >> *( lit('{')
				   >> expression_		[push_back(at_c<0>(_val), _1)]
				   >> lit('}')
				 )
			 >> lit('}')
			 >> -lit(';')
			 ;

		task_decl_.name("task_decl");
		task_list.name("task_list");
		task.name("task");
		pre_cond.name("pre_cond");
		post_cond.name("post_cond");
		cond.name("cond");

		qi::on_error<qi::fail> (task_decl_, error_handler(_4, _3, _2));
	}

	qi::rule<Iterator, task_decl_list_context(), white_space_> task_decl_;
	qi::rule<Iterator, task_decl_list(), white_space_> task_list;
	qi::rule<Iterator, task_decl(), white_space_> task;
	qi::rule<Iterator, cond_list_decl(), white_space_> pre_cond, post_cond, cond;

	grammar_expression<Iterator> expression_;
	scoped_identifier_grammar<Iterator> scoped_identifier;
};

bool parser::parse_expression(const std::string& expr)
{
	expression_ast result;
    bool r = parse(grammar_expression<std::string::const_iterator>(), expr, result);
	result.reduce();

	return r;
}	

bool parser::parse_task(const std::string& expr)
{
	task_decl_list_context result;
    bool r = parse(grammar_task<std::string::const_iterator>(), expr, result);

	if (r)
		return u.add_task(result);
	else
		return false;
}

bool parser::parse_task_file(const std::string& fileName)
{
	std::string content = read_from_file(fileName);
	return parse_task(content);
}
