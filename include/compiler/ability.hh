
#ifndef _COMPILER_ABILITY_HH
#define _COMPILER_ABILITY_HH

#include <compiler/symbols.hh>
#include <compiler/types.hh>

namespace hyper {
	namespace compiler {

		class ability : public boost::noncopyable {
			private:
				symbolList controlable_list;
				symbolList readable_list;
				symbolList private_list;

				std::string name;

			public:
				ability(const std::string& name_, const symbolList& controlables,
						const symbolList& readables, const symbolList& privates) :
							controlable_list(controlables),
							readable_list(readables),
							private_list(privates),
							name(name_) {};
		};
	};
};


#endif /* _COMPILER_ABILITY_HH */
