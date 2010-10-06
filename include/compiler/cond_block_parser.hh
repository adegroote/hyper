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
		        using namespace qi::labels;
				using phoenix::push_back;
				using phoenix::at_c;
				using phoenix::swap;
		
				start = pre_cond			[swap(at_c<0>(_val), _1)]
						>> post_cond			[swap(at_c<1>(_val), _1)]
						;
		
				pre_cond = lit("pre")
						 >> lit("=")
						 >> cond				[swap(at_c<0>(_val), at_c<0>(_1))]
						 ;
		
				post_cond = lit("post")
						  >> lit("=")
						  >> cond				[swap(at_c<0>(_val), at_c<0>(_1))]
						  ;
		
				cond = 
					 lit('{')
					 >> *( lit('{')
						   >> expression		[push_back(at_c<0>(_val), _1)]
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
			qi::rule<Iterator, cond_list_decl(), white_space_> cond, pre_cond, post_cond;
			grammar_expression<Iterator> expression;
		};
	}
}

#endif /* HYPER_COMPILER_COND_BLOCK_PARSER_HH_ */
