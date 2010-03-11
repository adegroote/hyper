
#ifndef _SYMBOLS_HH_
#define _SYMBOLS_HH_

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

		struct symbolList : public boost::noncopyable {
			public:
				enum symbolAddError {
					noError,
					alreadyExist,
					unknowType
				};

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
				std::pair < bool, symbolAddError> add(const std::string& name, const std::string& tname);

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
