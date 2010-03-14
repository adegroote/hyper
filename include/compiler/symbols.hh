
#ifndef _SYMBOLS_HH_
#define _SYMBOLS_HH_

#include <iostream>
#include <string>

#include <boost/noncopyable.hpp>

#include <compiler/types.hh>

namespace hyper {
	namespace compiler {

		struct symbol {
			std::string name;
			typeList::typeId t;

			symbol() {};
			symbol(const std::string name_, typeList::typeId t_) : name(name_), t(t_) {};
		};


		/* Structs used by parser */

		struct symbol_decl {
			std::string typeName;
			std::string name;
		};

		std::ostream& operator << (std::ostream& os, const symbol_decl&);

		struct symbol_decl_list {
			std::vector < symbol_decl> l;
		};

		std::ostream& operator << (std::ostream& os, const symbol_decl_list&);

		struct symbolList : public boost::noncopyable {
			public:
				enum symbolAddError {
					noError,
					alreadyExist,
					unknowType
				};

				typedef std::pair < bool, symbolAddError> add_result;

			private:
				typedef std::map< std::string, symbol> sMap;
				sMap symbols;
				const typeList & tlist; 

			public:

				symbolList(const typeList & lists_): tlist(lists_) {} ;
				/*
				 * Add a symbol to the symbol list
				 * < true, noError > means success
				 * < false, alreadyExist > means the symbol already exists
				 * < false, unknowType > means the type used to define the symbol is not know
				 */
				add_result add(const std::string& name, const std::string& tname);

				add_result add(const symbol_decl& s);

				std::vector<add_result> add(const symbol_decl_list& l);

				/*
				 * Retrieve a symbol from the symbol table
				 * < true, s > means success
				 * < false, _ > means the symbol does not exist in the table
				 */
				std::pair < bool, symbol > get(const std::string& name) const;
		};
	};
};

#endif /* _SYMBOLS_HH_ */
