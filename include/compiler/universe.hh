
#ifndef _COMPILER_UNIVERSE_HH_
#define _COMPILER_UNIVERSE_HH_

#include <hyperConfig.hh>

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/optional/optional.hpp>

#include <compiler/ability.hh>
#include <compiler/types.hh>
#include <compiler/functions_def.hh>

namespace hyper {
	namespace compiler {

		struct task_decl_list_context;
		struct ability_decl;
		class ability;

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

				bool add_task(const task_decl_list_context& decl);

				std::pair<bool, symbolACL> 
				get_symbol(const std::string& name, const ability&) const;

				std::pair<bool, functionDef>
				get_functionDef(const std::string& name) const;

				boost::optional<typeId>
				typeOf(const ability&, const expression_ast& expr) const;

				const typeList& types() const { return tList; };

				size_t dump_ability_types(std::ostream& os, const std::string& abilityName) const;
				size_t dump_ability_functions_proto(std::ostream& oss, const std::string& name) const;
				size_t dump_ability_functions_impl(std::ostream& oss, const std::string& name) const;
				void dump_ability(std::ostream& oss, const std::string& name) const;
		};
	}
}

#endif
