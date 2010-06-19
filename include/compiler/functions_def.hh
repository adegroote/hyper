#ifndef _FUNCTIONS_DEF_HH_
#define _FUNCTIONS_DEF_HH_

#include <cassert>
#include <string>
#include <sstream>
#include <stdexcept>

#include <compiler/functions_def_parser.hh>
#include <compiler/types.hh>

#include <boost/tuple/tuple.hpp>

namespace hyper {
	namespace compiler {
		class functionDef 
		{
			private:
				std::string name_;
				std::vector < typeList::typeId > argsType_;
				typeList::typeId returnType_;

			public:
				functionDef() {};
				functionDef(const std::string &name, 
							const std::vector < typeList::typeId > & args,
							typeList::typeId returns):
					name_(name), argsType_(args), returnType_(returns) 
				{};

				const std::string name() const {
					return name_;
				}

				size_t arity() const {
					return argsType_.size();
				};
				
				typeList::typeId returnType() const {
					return returnType_;
				}

				typeList::typeId argsType(size_t i) const {
					assert(i < arity());
					return argsType_[i];
				};

				void output_proto(std::ostream& os, const typeList& tList) const;
				void output_impl(std::ostream& os, const typeList& tList) const;
		};

		class functionDefList : public boost::noncopyable
	   	{
			private:
				typedef std::map < std::string, functionDef > fmap;
				fmap functionDefs;
				const typeList& tlist;

			public:
				enum addErrorType {
					noError,
					alreadyExist,
					unknowReturnType,
					unknowArgsType
				};

				typedef boost::tuple<bool, addErrorType, int> add_result;

				functionDefList(const typeList & list_) : tlist(list_) {};
				/*
				 * Add a function definition to the list of known function
				 * Functions are identified by their name currently, no fancy feature
				 * return < true, _, _ > in case of success
				 * return < false, alreadyExist, _ > if we try to redefine a
				 * function
				 * return < false, unknowReturnType, _ > if we try to use an
				 * undefined return type
				 * return < false, unknowArgsType, i > if the args i uses an
				 * undefined type
				 */
				add_result
				add(const std::string& name, const std::string& return_name,
					const std::vector< std::string > & argsName);

				add_result add(const function_decl& decl);

				std::string 
			    get_diagnostic(const function_decl& decl, const add_result& res);

				std::vector<add_result> add(const function_decl_list &l);

				/*
				 * Return the definition of a function, knowing its name
				 * return < true, def > if the function exists
				 * return < false, _ > otherwise
				 */
				std::pair < bool, functionDef > get(const std::string&) const;

				template <typename Pred>
				std::vector<functionDef> select(Pred pred) const
				{
					std::vector<functionDef> res;
					fmap::const_iterator it;
					for (it = functionDefs.begin(); it != functionDefs.end(); ++it)
						if (pred(it->second))
							res.push_back(it->second);
					return res;
				}
		};
	}
}

#endif /* _FUNCTIONS_DEF_HH_ */
