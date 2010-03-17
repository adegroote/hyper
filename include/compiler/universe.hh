
#ifndef _COMPILER_UNIVERSE_HH_
#define _COMPILER_UNIVERSE_HH_

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <compiler/ability.hh>
#include <compiler/types.hh>
#include <compiler/functions_def.hh>
#include <compiler/ability_parser.hh>

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
				 * Check if is part of native identifier
				 */
				bool is_basic_identifier (const std::string &) const ;

				bool add_types(const std::string&, const type_decl_list& decl);

				bool add_functions(const std::string&, const function_decl_list& decl);

			public:
				universe();

				bool add(const ability_decl& decl);

				/* 
				 * Automaticly add the scope to the identifier
				 * Don't do the promotion if the identifier is already scoped
				 * or is a basic identifier
				 */
				std::string add_scope(const std::string& abilityName,
								      const std::string& identifier) const;

		};
	};
};

#endif
