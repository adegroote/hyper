
#ifndef _COMPILER_ABILITY_HH
#define _COMPILER_ABILITY_HH

#include <compiler/symbols.hh>
#include <compiler/types.hh>

namespace hyper {
	namespace compiler {

		class ability : public boost::noncopyable {
			private:
				typeList & tList;
				symbolList controlable_list;
				symbolList readable_list;
				symbolList private_list;

				std::string name;

			public:
				ability(typeList& tlist, const std::string &name_) :
					tList(tList), controlable_list(tList), 
					readable_list(tList), private_list(tList),
					name(name_) {};
					
		};
	};
};


#endif /* _COMPILER_ABILITY_HH */
