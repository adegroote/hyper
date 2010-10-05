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
