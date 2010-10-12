#ifndef HYPER_COMPILER_COND_BLOCK_PARSER_HH_
#define HYPER_COMPILER_COND_BLOCK_PARSER_HH_

#include <compiler/base_parser.hh>
#include <compiler/task_parser.hh>

BOOST_FUSION_ADAPT_STRUCT(
	hyper::compiler::cond_list_decl,
	(std::vector<hyper::compiler::expression_ast>, list)
)

BOOST_FUSION_ADAPT_STRUCT(
	hyper::compiler::cond_block_decl,
	(hyper::compiler::cond_list_decl, pre)
	(hyper::compiler::cond_list_decl, post)
)

namespace hyper {
	namespace compiler {
		template <typename Iterator>
		struct cond_block_grammar :
			qi::grammar<Iterator, cond_block_decl(), white_space<Iterator> >
		{
			typedef white_space<Iterator> white_space_;
		
			cond_block_grammar() :
				cond_block_grammar::base_type(start)
			{
				using qi::lit;
		
				start = pre_cond
						>> post_cond
						;
		
				pre_cond = lit("pre")
						 >> lit("=")
						 >> cond
						 ;
		
				post_cond = lit("post")
						  >> lit("=")
						  >> cond
						  ;
		
				cond = 
					 lit('{')
					 >> *( lit('{')
						   >> expression
						   >> lit('}')
						 )
					 >> lit('}')
					 >> -lit(';')
					 ;
		
				start.name("block cond");
				pre_cond.name("pre_cond");
				post_cond.name("post_cond");
				cond.name("cond");
			}
		
			qi::rule<Iterator, cond_block_decl(), white_space_> start;
			qi::rule<Iterator, cond_list_decl(), white_space_> pre_cond, post_cond;
			qi::rule<Iterator, std::vector<expression_ast>(), white_space_> cond;
			grammar_expression<Iterator> expression;
		};
	}
}

#endif /* HYPER_COMPILER_COND_BLOCK_PARSER_HH_ */
