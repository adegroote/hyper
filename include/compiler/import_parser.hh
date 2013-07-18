#include <compiler/parser.hh>
#include <compiler/base_parser.hh>

#include <boost/optional/optional.hpp>



namespace hyper {
	namespace compiler {
		struct import_exception {
			std::string filename;

			import_exception(const std::string& filename_) : filename(filename_) {}

			virtual const char* what() = 0;
		};

		struct import_exception_not_found : public import_exception
		{
			import_exception_not_found(const std::string& filename) :
				import_exception(filename)
			{}

			virtual const char* what() {
				std::ostringstream oss;
				oss << "Can't find the file " << filename;
				oss << " : please fix the include path " << std::endl;
				return oss.str().c_str();
			}
		};

		struct import_exception_parse_error : public import_exception
		{
			import_exception_parse_error(const std::string& filename) :
				import_exception(filename)
			{}

			virtual const char* what() {
				std::ostringstream oss;
				oss << "Parse error in file " << filename << std::endl;
				return oss.str().c_str();
			}
		};
	}
}

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
