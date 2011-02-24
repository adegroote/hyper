#include <compiler/parser.hh>
#include <compiler/base_parser.hh>

#include <boost/optional/optional.hpp>


namespace fs = boost::filesystem;
typedef boost::optional<fs::path> optional_path;

struct gen_path 
{
	std::string filename;

	gen_path(const std::string& filename_) : filename(filename_) {}

	fs::path operator() (const fs::path & path) const {
		fs::path computed_path = path / filename;
		return computed_path;
	}
};

struct path_exist
{
	bool operator() (const fs::path& path) const
	{
		return fs::exists(path);
	}
};

static inline optional_path 
search_ability_file(hyper::compiler::parser& p, const std::string& filename)
{
	if (fs::exists(filename))
		return fs::path(filename);

	std::vector<fs::path> generated_path;
	std::transform(p.include_begin(), p.include_end(),
				   std::back_inserter(generated_path),
				   gen_path(filename));

	std::vector<fs::path>::const_iterator it;
	it = std::find_if(generated_path.begin(), generated_path.end(), path_exist());

	if (it != generated_path.end())
		return *it;

	return boost::none;
}

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
		optional_path ability_path = search_ability_file(p, filename);
		if (! ability_path ) 
			throw hyper::compiler::import_exception_not_found(filename);

		bool res = p.parse_ability_file((*ability_path).string());
		if (!res) 
			throw hyper::compiler::import_exception_parse_error((*ability_path).string());
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
					>> constant_string
					>> -qi::lit(';')
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
