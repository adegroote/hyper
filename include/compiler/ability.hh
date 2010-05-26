
#ifndef _COMPILER_ABILITY_HH
#define _COMPILER_ABILITY_HH

#include <compiler/symbols.hh>
#include <compiler/types.hh>

namespace hyper {
	namespace compiler {

		enum abilityACL {
			PRIVATE,
			READABLE,
			CONTROLABLE
		};

		struct symbolACL {
			symbol s;
			abilityACL acl;

			symbolACL() {};
			symbolACL(const symbol& s_, abilityACL acl_) : s(s_), acl(acl_) {};
		};

		class ability : public boost::noncopyable {
			private:
				symbolList controlable_list;
				symbolList readable_list;
				symbolList private_list;

				std::string name_;

			public:
				ability(const std::string& name, const symbolList& controlables,
						const symbolList& readables, const symbolList& privates) :
							controlable_list(controlables),
							readable_list(readables),
							private_list(privates),
							name_(name) {};

				/*
				 * Look for a symbol + permission in an ability
				 * By construction, it exists only zero or one occurence of the
				 * symbol in the ability
				 * @Returns <false,_> if not found
				 * @returns <true, symbolACL> if found
				 */
				std::pair<bool, symbolACL> get_symbol(const std::string&) const;

				const std::string name() const
				{
					return name_;
				}
		};
	};
};


#endif /* _COMPILER_ABILITY_HH */
