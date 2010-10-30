#ifndef _TYPES_HH_
#define _TYPES_HH_

#include <map>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/tuple/tuple.hpp>

#include <compiler/utils.hh>
#include <compiler/types_parser.hh>

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

		namespace detail {
			typedef std::size_t typeId;
		}

		struct Nothing {};

		class symbolList;
		class typeList;

		typedef boost::variant< Nothing, 
								detail::typeId,
								boost::shared_ptr<symbolList>
							  > typeInternal;

		struct type {
			std::string name;
			typeOfType t;
			typeInternal internal;

			type(const std::string& name_, typeOfType t_): name(name_), t(t_), internal(Nothing()) {};

			void output(std::ostream& oss, const typeList& tList) const;
		};

		class typeList : public boost::noncopyable {
			public:
				typedef detail::typeId typeId;

			private:
				typedef std::vector<type> typeV;
				typedef std::map<std::string, typeId> typeMap;

				typeV types;
				typeMap mapsId; 

			public:


				// XXX duplicate part of symbol error value ...
				enum addTypeError {
					noError, 
					notTested,
					typeAlreadyExist,
					symAlreadyExist,
					unknowType
				};

				struct native_decl_error {
					addTypeError err;
				};

				struct struct_decl_error {
					addTypeError local_err;
					std::vector < addTypeError > var_err;
				};

				struct new_decl_error {
					addTypeError new_name_error;
					addTypeError old_name_error;
				};

				typedef boost::variant< native_decl_error, struct_decl_error, new_decl_error > 
						add_error;

				typedef boost::tuple<bool, typeList::typeId, add_error> add_result;

				typeList() {};
				/* 
				 * Add a type to the typeList
				 * return < true, id> if the type is really created
				 * return < false, id> if the type already exists
				 */
				add_result add(const std::string & name, typeOfType t);

				add_result add(const type_decl& decl);

				std::vector<add_result> add(const type_decl_list& list);

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

				type& get(typeList::typeId);
				
				template <typename Pred>
				std::vector<type> select(Pred pred) const
				{
					std::vector<type> res;
					hyper::compiler::copy_if(types.begin(), types.end(), std::back_inserter(res), pred);
					return res;
				}
		};
	}
}

#endif /* _TYPES_HH_ */
