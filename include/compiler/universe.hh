
#ifndef _COMPILER_UNIVERSE_HH_
#define _COMPILER_UNIVERSE_HH_

#include <hyperConfig.hh>

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/optional/optional.hpp>

#include <compiler/ability.hh>
#include <compiler/depends.hh>
#include <compiler/types.hh>
#include <compiler/functions_def.hh>

namespace hyper {
	namespace compiler {

		struct task_decl_list_context;
		struct ability_decl;
		class ability;
		struct recipe_expression;

		class universe : public boost::noncopyable
		{
			private:
				typedef std::map< std::string, boost::shared_ptr<ability> > abilityMap;
				abilityMap abilities;

				typeList tList;
				functionDefList fList;

				/*
				 * Check if is part of native identifier
				 */
				bool is_basic_identifier (const std::string &) const ;

				bool add_types(const std::string&, const type_decl_list& decl);

				bool add_functions(const std::string&, const function_decl_list& decl);

				bool add_symbols(const std::string&, const symbol_decl_list& decl, 
						symbolList& s);


			public:
				universe();

				bool add(const ability_decl& decl);

				std::pair<bool, symbolACL> 
				get_symbol(const std::string& name, const ability&, 
						   const boost::optional<symbolList>& s = boost::none) const;

				std::pair<bool, functionDef>
				get_functionDef(const std::string& name) const;

				boost::optional<typeId>
				typeOf(const ability&, const expression_ast& expr,
					   const boost::optional<symbolList>&s = boost::none) const;

				boost::optional<typeId>
				typeOf(const ability&, const recipe_expression& expr,
					  const boost::optional<symbolList>&s = boost::none) const;

				const typeList& types() const { return tList; };
				const functionDefList& funcs() const { return fList; };

				size_t dump_ability_types(std::ostream& os, const std::string& abilityName) const;
				size_t dump_swig_ability_types(std::ostream& os, const std::string& abilityName) const;
				size_t dump_ability_functions_proto(std::ostream& oss, const std::string& name) const;
				size_t dump_ability_functions_impl(std::ostream& oss, const std::string& name) const;
				void dump_ability_import_module_def(std::ostream& oss, const std::string& name) const;
				void dump_ability_import_module_impl(std::ostream& oss, const std::string& name) const;
				void dump_ability(std::ostream& oss, const std::string& name) const;
				depends get_function_depends(const std::string& name) const;

				ability& get_ability(const std::string& name);
				const ability& get_ability(const std::string& name) const;
		};
	}
}

#endif
