#ifndef _TYPES_HH_
#define _TYPES_HH_

#include <map>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

namespace hyper {
	namespace compiler {

		enum typeOfType {
			noType,
			intType,
			boolType,
			doubleType,
			stringType,
			enumType, 
			structType
		};

		struct type {
			std::string name;
			typeOfType t;

			type(const std::string& name_, typeOfType t_): name(name_), t(t_) {};
		};

		class typeList : public boost::noncopyable {
			public:
				typedef std::size_t typeId;

			private:
				typedef std::vector<type> typeV;
				typedef std::map<std::string, typeId> typeMap;

				typeV types;
				typeMap mapsId; 

			public:
				typeList() {};
				/* 
				 * Add a type to the typeList
				 * return < true, id> if the type is really created
				 * return < false, id> if the type already exists
				 */
				std::pair < bool, typeList::typeId > add(const std::string & name, typeOfType t);

				/*
				 * Get a typeId of a type on the base of its name
				 * return < true, id> if we find a type with this name
				 * return < false, _> otherwise
				 */
				std::pair < bool, typeList::typeId > getId(const std::string & name) const;

				/*
				 * Return the real type associated to its id
				 * Pre : id is a valid typeId, from add or getId
				 */
				type get(typeList::typeId id) const;
		};
	};
};

#endif /* _TYPES_HH_ */
