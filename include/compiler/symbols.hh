
#ifndef _SYMBOLS_HH_
#define _SYMBOLS_HH_

#include <string>

#include <compiler/types.hh>

namespace hyper {
	namespace compiler {

		struct symbol {
			std::string name;
			typeList::typeId t;

			symbol() {};
			symbol(const std::string name_, typeList::typeId t_) : name(name_), t(t_) {};
		};

		struct symbolList {
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
				std::pair < bool, symbolAddError> add(const std::string& name, const std::string& tname);
				std::pair < bool, symbol > get(const std::string& name);
		};
	};
};

#endif /* _SYMBOLS_HH_ */
