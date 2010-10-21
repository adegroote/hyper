#include <compiler/parser.hh>
#include <compiler/base_parser.hh>

struct parse_import {

	hyper::compiler::parser &p;

	template <typename>
	struct result { typedef bool type; };

	parse_import(hyper::compiler::parser & p_) : p(p_) {};

	bool operator()(const std::string& filename) const
	{
		return p.parse_ability_file(filename);
	}
};

namespace hyper {
	namespace compiler {
		template<typename Iterator>
			struct import_grammar: qi::grammar<Iterator, white_space<Iterator> >
		{
			typedef white_space<Iterator> white_space_;

			import_grammar(hyper::compiler::parser& p) : 
				import_grammar::base_type(start), import_adder(p)
			{
				start =
					*(import				[import_adder(qi::labels::_1)])
					;

				import =	qi::lit("import")
					>> constant_string
					>> -qi::lit(';')
					;

				start.name("import list");
				import.name("import");
			}

			qi::rule<Iterator, white_space_> start;
			qi::rule<Iterator, std::string(), white_space_> import;
			phoenix::function<parse_import> import_adder;
			const_string_grammer<Iterator> constant_string;
		};
	}
}
