#ifndef HYPER_COMPILER_BASE_PARSER_HH_
#define HYPER_COMPILER_BASE_PARSER_HH_

#include <iostream>
#include <string>
#include <vector>

#include <compiler/expression_ast.hh>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_algorithm.hpp>
#include <boost/spirit/include/phoenix_bind.hpp> 
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

BOOST_FUSION_ADAPT_STRUCT(
		hyper::compiler::function_call,
		(std::string, fName)
		(std::vector<hyper::compiler::expression_ast>, args)
)

namespace {
	struct generate_scoped_identifier {
		template <typename, typename>
			struct result { typedef std::string type; };

		std::string operator()(const std::string& scope, 
				const std::string& identifier) const
		{
			return scope + "::" + identifier;
		}
	};
}

namespace hyper {
	namespace compiler {
		struct error_handler_
		{
			template <typename, typename, typename>
			struct result { typedef void type; };

			template <typename Iterator>
			void operator()(
					qi::info const& what
					,Iterator err_pos, Iterator last) const
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

		static boost::phoenix::function<error_handler_> const error_handler = error_handler_();

		template <typename Iterator>
		struct white_space : qi::grammar<Iterator>
		{
			white_space() : white_space::base_type(start)
			{
				using boost::spirit::ascii::char_;
				using namespace qi::labels;

				start =
					qi::space                               // tab/space/cr/lf
					|   "/*" >> *(char_ - "*/") >> "*/"
					|   "//" >> *(char_)
					;

				start.name("ws");
				qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
			}

			qi::rule<Iterator> start;
		};

		template<typename Iterator>
		struct identifier_grammar: qi::grammar<Iterator, std::string()>
		{
			identifier_grammar() : identifier_grammar::base_type(start)
			{
				using namespace qi::labels;
				start = qi::raw[qi::lexeme[(qi::alpha | '_') >> *(qi::alnum | '_')]];

				start.name("identifier");
				qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
			}

			qi::rule<Iterator, std::string()> start;
		};

		template <typename Iterator>
		struct scoped_identifier_grammar: qi::grammar<Iterator, std::string()>
		{
			scoped_identifier_grammar() : scoped_identifier_grammar::base_type(start)
			{
				using namespace qi::labels;
				start = id		[_val = _1]
					> -(*("::"	
								> id		[_val = gen(_val, _1)]
						  )
						)
					;

				start.name("scoped_identifier");
				qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
			}

			qi::rule<Iterator, std::string()> start;
			phoenix::function<generate_scoped_identifier> gen;
			identifier_grammar<Iterator> id;
		};

		template <typename Iterator>
		struct const_string_grammer: qi::grammar<Iterator, std::string()>
		{
			const_string_grammer() : const_string_grammer::base_type(start)
			{
				using namespace qi::labels;
				start = '"' 
					> *(qi::lexeme[qi::char_ - '"'] [_val+= _1]) 
					> '"'
					;

				start.name("const_string");
				qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
			}

			qi::rule<Iterator, std::string()> start;
		};

		template <typename Iterator>
		struct constant_grammar :
			qi::grammar<Iterator, expression_ast(), white_space<Iterator> >
		{
			typedef white_space<Iterator> white_space_;

			constant_grammar() : constant_grammar::base_type(start)
			{
				qi::real_parser<double, qi::strict_ureal_policies<double> > strict_double;
				using namespace qi::labels;
				using phoenix::at_c;

				start = 
						(
						cst_double			   
						| cst_int				   
						| cst_string			   
						| cst_bool
						);

				cst_int = qi::uint_;
				cst_double = strict_double;
				cst_string = constant_string;
				cst_bool = qi::bool_;

				start.name("constant");
				cst_int.name("const int");
				cst_double.name("const double");
				cst_string.name("const string");
				cst_bool.name("const bool");
				qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
			}

			const_string_grammer<Iterator> constant_string;
			qi::rule<Iterator, expression_ast(), white_space_> start;
			qi::rule<Iterator, Constant<int>(), white_space_> cst_int;
			qi::rule<Iterator, Constant<double>(), white_space_> cst_double;
			qi::rule<Iterator, Constant<bool>(), white_space_> cst_bool;
			qi::rule<Iterator, Constant<std::string>(), white_space_> cst_string;
		};

		template <typename Iterator>
		struct atom_grammar :
			qi::grammar<Iterator, expression_ast(), white_space<Iterator> >
		{
			typedef white_space<Iterator> white_space_;

			atom_grammar() : atom_grammar::base_type(start)
			{
				using namespace qi::labels;
				start =   constant
						| var_inst 			   
						;

				var_inst = scoped_identifier;

				start.name("atom");
				var_inst.name("var instance");
				qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
			}

			qi::rule<Iterator, expression_ast(), white_space_> start;
			qi::rule<Iterator, std::string(), white_space_> var_inst;

			constant_grammar<Iterator> constant;
			scoped_identifier_grammar<Iterator> scoped_identifier;
		};

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
				using phoenix::bind;
		
				expression =
					equality_expr.alias()
						;
		
				equality_expr =
					relational_expr								[_val = _1]
					> *(	("==" > relational_expr      [bind(&expression_ast::binary<EQ>, _val, _1)])
						|   ("!=" > relational_expr     [bind(&expression_ast::binary<NEQ>, _val, _1)])
						)
					;
		
				relational_expr =
					logical_expr								[_val = _1]
					> *(	("<=" > logical_expr        [bind(&expression_ast::binary<LTE>, _val, _1)])
						|   ('<' > logical_expr         [bind(&expression_ast::binary<LT>, _val, _1)])
						|   (">=" > logical_expr        [bind(&expression_ast::binary<GTE>, _val, _1)])
						|   ('>' > logical_expr         [bind(&expression_ast::binary<GT>, _val, _1)])
						)
					;
		
				logical_and_expr =
					additive_expr								[_val = _1]
					> *(  "&&" > additive_expr			[bind(&expression_ast::binary<AND>, _val, _1)])
					;
		
				logical_or_expr =
					logical_and_expr							[_val = _1]
					> *("||" > logical_and_expr			[bind(&expression_ast::binary<OR>, _val, _1)])
					;
		
				logical_expr = logical_or_expr.alias()
					;
						
				additive_expr =
					multiplicative_expr						[_val = _1]
					> *(	('+' > multiplicative_expr	[bind(&expression_ast::binary<ADD>, _val, _1)])
						|   ('-' > multiplicative_expr  [bind(&expression_ast::binary<SUB>, _val, _1)])
						)
					;
		
				multiplicative_expr =
					unary_expr							[_val = _1]
					> *(	('*' > unary_expr		[bind(&expression_ast::binary<MUL>, _val, _1)])
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
					    |	(atom							[_val = _1])
						|   ('(' > expression				[_val = _1] > ')')
					;
		
		
		
				func_call = scoped_identifier [at_c<0>(_val) = _1]
						  >> lit('(')
						  > -(
							   expression		[push_back(at_c<1>(_val), _1)]
							   > *( ',' >
								     expression	[push_back(at_c<1>(_val), _1)])
							  )
						  > lit(')')
						  ;
		
				expression.name("expression declaration");
				primary_expr.name("primary expr declaration");
				unary_expr.name("unary expr declaration");
				multiplicative_expr.name("multiplicative expr declaration");
				additive_expr.name("additive expr declaration");
				relational_expr.name("relational expr declaration");
				equality_expr.name("equality expr declaration");
				func_call.name("function declaration instance");

				qi::on_error<qi::fail> (expression, error_handler(_4, _3, _2));
			}
		
			qi::rule<Iterator, expression_ast(), white_space_> expression, primary_expr, unary_expr;
			qi::rule<Iterator, expression_ast(), white_space_> multiplicative_expr, additive_expr;
			qi::rule<Iterator, expression_ast(), white_space_> logical_expr;
			qi::rule<Iterator, expression_ast(), white_space_> logical_and_expr, logical_or_expr;
		    qi::rule<Iterator, expression_ast(), white_space_> relational_expr, equality_expr;
			qi::rule<Iterator, function_call(), white_space_> func_call;
		
			constant_grammar<Iterator> constant;
			scoped_identifier_grammar<Iterator> scoped_identifier;
			atom_grammar<Iterator> atom;
		};

		template <typename Grammar>
		bool parse(const Grammar& g, const std::string& expr)
		{
			typedef white_space<std::string::const_iterator> white_space;
			white_space ws;

			std::string::const_iterator iter = expr.begin();
			std::string::const_iterator end = expr.end();
			bool r = phrase_parse(iter, end, g, ws);

			return (r && iter == end);
		}

		template <typename Grammar, typename T>
		bool parse(const Grammar& g, const std::string& expr, T& res)
		{
			typedef white_space<std::string::const_iterator> white_space;
			white_space ws;

			std::string::const_iterator iter = expr.begin();
			std::string::const_iterator end = expr.end();
			bool r = phrase_parse(iter, end, g, ws, res);

			return (r && iter == end);
		}
	}
}

#endif /* HYPER_COMPILER_BASE_PARSER_HH_ */
