
#ifndef _COMPILER_UNIVERSE_HH_
#define _COMPILER_UNIVERSE_HH_

#include <hyperConfig.hh>

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <compiler/ability.hh>
#include <compiler/types.hh>
#include <compiler/functions_def.hh>
#include <compiler/ability_parser.hh>
#include <compiler/task_parser.hh>

namespace hyper {
	namespace compiler {

		class universe : public boost::noncopyable
		{
			private:
				typedef std::map< std::string, boost::shared_ptr<ability> > abilityMap;
				abilityMap abilities;

				typeList tList;
				functionDefList fList;

				std::vector<std::string> basicIdentifier;

				/*
				 * Check if it is a scoped identifier
				 */
				bool is_scoped_identifier( const std::string &) const;

				/*
				 * Return a pair of abilityName and variableName
				 * assume it is a scoped identifier
				 */
				std::pair<std::string, std::string> decompose(const std::string& name) const;

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

				/* 
				 * Automaticly add the scope to the identifier
				 * Don't do the promotion if the identifier is already scoped
				 * or is a basic identifier
				 */
				std::string add_scope(const std::string& abilityName,
								      const std::string& identifier) const;

				std::pair<bool, symbolACL> 
				get_symbol(const std::string& name, const boost::shared_ptr<ability>& pAbility) const;

				std::pair<bool, functionDef>
				get_functionDef(const std::string& name) const;

				typeId typeOf(const boost::shared_ptr<ability>&, const expression_ast& expr) const;

				const typeList& types() const { return tList; };
		};
	};
};

#endif
