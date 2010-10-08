#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include <compiler/expression_ast.hh>
#include <compiler/parser.hh>
#include <compiler/base_parser.hh>
#include <compiler/task_parser.hh>
#include <compiler/cond_block_parser.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>

//#define HYPER_DEBUG_RULES

using boost::phoenix::function;
using boost::phoenix::ref;
using boost::phoenix::size;

using namespace hyper::compiler;

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;

std::ostream& hyper::compiler::operator << (std::ostream& os, const cond_list_decl& l)
{
	std::copy(l.list.begin(), l.list.end(), std::ostream_iterator<expression_ast>( os ));
	return os;
}

std::ostream& hyper::compiler::operator<< (std::ostream& os, const cond_block_decl& c)
{
	os << "{" << c.pre << "}";
	os << "{" << c.post << "}";;
}

std::ostream& hyper::compiler::operator << (std::ostream& os, const task_decl& t)
{
	os << t.name;
	os << t.conds;
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
	task_decl,
	(std::string, name)
	(cond_block_decl, conds)
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
			 >> cond_block				[swap(at_c<1>(_val), _1)]
			 >> lit('}')
			 >> -lit(';')
			 ;

		task_decl_.name("task_decl");
		task_list.name("task_list");
		task.name("task");

		qi::on_error<qi::fail> (task_decl_, error_handler(_4, _3, _2));
	}

	qi::rule<Iterator, task_decl_list_context(), white_space_> task_decl_;
	qi::rule<Iterator, task_decl_list(), white_space_> task_list;
	qi::rule<Iterator, task_decl(), white_space_> task;

	scoped_identifier_grammar<Iterator> scoped_identifier;
	cond_block_grammar<Iterator> cond_block;
};

bool parser::parse_expression(const std::string& expr)
{
	expression_ast result;
	grammar_expression<std::string::const_iterator> g;
    bool r = parse(g, expr, result);
	result.reduce();

	return r;
}	

bool parser::parse_task(const std::string& expr)
{
	task_decl_list_context result;
	grammar_task<std::string::const_iterator> g;
    bool r = parse(g, expr, result);

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
