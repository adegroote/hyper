
#ifndef _SYMBOLS_HH_
#define _SYMBOLS_HH_

#include <iostream>
#include <map>
#include <string>

#include <boost/noncopyable.hpp>

#include <compiler/types.hh>
#include <compiler/expression_ast.hh>

namespace hyper {
	namespace compiler {

		typedef std::size_t typeId;

		struct symbol {
			std::string name;
			typeList::typeId t;
			expression_ast initializer;

			symbol() {};
			symbol(const std::string& name_, typeList::typeId t_, 
				   const expression_ast& initializer_ = empty()) : 
				name(name_), t(t_), initializer(initializer_) {};
		};

		struct symbol_decl;
		struct symbol_decl_list;

		struct symbolList {
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
				add_result add(const std::string& name, const std::string& tname, 
							   const expression_ast& initializer = empty());

				add_result add(const symbol_decl& s);

				std::vector<add_result> add(const symbol_decl_list& l);

				std::string get_diagnostic(const symbol_decl& decl, const add_result& res);

				/*
				 * Retrieve a symbol from the symbol table
				 * < true, s > means success
				 * < false, _ > means the symbol does not exist in the table
				 */
				std::pair < bool, symbol > get(const std::string& name) const;

				/*
				 * Returns a list of commun symbol into the two symbolList (this and s)
				 */
				std::vector<std::string> intersection(const symbolList& s) const;

				/* 
				 * Provide a const_iterator on symbol list
				 * it is a const iterator on std::pair<std::string, symbol>
				 */
				typedef std::map<std::string, symbol>::const_iterator const_iterator;
				const_iterator begin() const { return symbols.begin(); };
				const_iterator end() const { return symbols.end(); };
		};
	}
}

#endif /* _SYMBOLS_HH_ */
