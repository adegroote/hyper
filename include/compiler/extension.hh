#ifndef HYPER_COMPILER_EXTENSION_HH_
#define HYPER_COMPILER_EXTENSION_HH_

#include <sstream>

namespace hyper {
	namespace compiler {
		struct functionDef;
		struct typeList;
		class universe;
		class ability;
		class task;
		class symbolList;
		struct expression_ast;

		struct extension {

			virtual void function_proto(std::ostream& oss, const functionDef& f,
												   const typeList& tList) const = 0;

			virtual void function_proto_additional_includes(std::ostream& oss, 
													const std::string& name) const = 0;

			virtual void function_impl(std::ostream& oss, const functionDef& f,
												   const typeList& tList) const = 0;

			virtual void output_import(std::ostream& oss, const functionDef& f,
														  const typeList& tList) const = 0;

			virtual void recipe_additional_includes(std::ostream& oss,
												    const std::string& name) const = 0;

			virtual void dump_eval_expression(std::ostream& oss, 
											  const universe& u,
											  const ability& a,
											  const task& t,
											  const std::string& local_data_type,
											  const symbolList& local_symbol,
											  size_t counter,
											  const expression_ast& e) const = 0;

			virtual void generate_expression_caller(std::ostream& oss,
												    size_t counter) const = 0;

			virtual std::string generate_abortable_function(const universe&,
															const ability&,
															const task&,
															const std::string& type_locals,
															size_t counter, 
															const std::string& target_var) const = 0;

			virtual ~extension() {}
		};

		std::pair<bool, std::string> 
		is_tagged_func_call(const expression_ast& e, const std::string& name, const universe &u);
	}
}

#endif /* HYPER_COMPILER_EXTENSION_HH_ */
