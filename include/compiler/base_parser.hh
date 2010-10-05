#ifndef HYPER_COMPILER_BASE_PARSER_HH_
#define HYPER_COMPILER_BASE_PARSER_HH_

#include <iostream>
#include <string>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_algorithm.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

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
			}

			qi::rule<Iterator> start;
		};

		template<typename Iterator>
		struct identifier_grammar: qi::grammar<Iterator, std::string()>
		{
			identifier_grammar() : identifier_grammar::base_type(start)
			{
				using namespace qi::labels;
				start = (qi::alpha | '_') >> *(qi::alnum | '_');

				start.name("identifier");
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
					>> -(*("::"	
								>> id		[_val = gen(_val, _1)]
						  )
						)
					;

				start.name("scoped_identifier");
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
					>> *(qi::lexeme[qi::char_ - '"'] [_val+= _1]) 
					>> '"'
					;
			}

			qi::rule<Iterator, std::string()> start;
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
