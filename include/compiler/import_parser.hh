#include <compiler/parser.hh>
#include <compiler/base_parser.hh>
#include <compiler/import_exception.hh>

#include <boost/optional/optional.hpp>



struct parse_import {

	hyper::compiler::parser &p;

	template <typename>
	struct result { typedef void type; };

	parse_import(hyper::compiler::parser & p_) : p(p_) {};

	void operator()(const std::string& filename) const
	{
		bool res = p.parse_ability_file(filename);
		if (!res) 
			throw hyper::compiler::import_exception_parse_error(filename);
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
		
				using namespace qi::labels;
				start =
					*(import				[import_adder(_1)])
					;

				import =	qi::lit("import")
					> constant_string
					> -qi::lit(';')
					;

				start.name("import list");
				import.name("import");
				qi::on_error<qi::fail> (start, error_handler(_4, _3, _2));
			}

			qi::rule<Iterator, white_space_> start;
			qi::rule<Iterator, std::string(), white_space_> import;
			phoenix::function<parse_import> import_adder;
			const_string_grammer<Iterator> constant_string;
		};
	}
}
