
#ifndef _HYPER_COMPILER_TASK_HH_
#define _HYPER_COMPILER_TASK_HH_ 

#include <iostream>
#include <string>
#include <vector>

#include <compiler/expression_ast.hh>

namespace hyper {
	namespace compiler {

		class ability;
		class universe;
		class typeList;
		struct task_decl;

		class task {
			private:
				std::string name;
				std::vector<expression_ast> pre;
				std::vector<expression_ast> post;
				
				const ability & ability_context;
				const typeList& tList;

			public:
				task(const task_decl&, const ability&, const typeList&);
				bool validate(const universe& u) const;
				void dump(std::ostream& oss, const universe& u) const;
				void dump_include(std::ostream& oss, const universe& u) const;

				const std::string& get_name() const { return name; };

				typedef std::vector<expression_ast>::const_iterator const_iterator;
				const_iterator pre_begin() const { return pre.begin(); }
				const_iterator pre_end() const { return pre.end(); };
				const_iterator post_begin() const { return post.begin(); }
				const_iterator post_end() const { return post.end(); };
		};

		std::ostream& operator<< (std::ostream& oss, const task& t);
	}
}

#endif /* _HYPER_COMPILER_TASK_HH_ */
