
#ifndef _COMPILER_SCOPE_HH_
#define _COMPILER_SCOPE_HH_

#include <vector>
#include <string>

namespace hyper {
	namespace compiler {
		class scope {
			private:
				static std::vector<std::string> basicIdentifier;
				scope();

			public:
				/*
				 * Check if the identifiant is a scoped identifier
				 */
				static bool is_scoped_identifier(const std::string&);

				/*
				 * Check if the identifier is a basic identifier / keyword
				 */
				static bool is_basic_identifier(const std::string&);
				/* 
				 * Automaticly add the scope to the identifier
				 * Don't do the promotion if the identifier is already scoped
				 * or is a basic identifier
				 */
				static std::string add_scope(const std::string&, const std::string&);

				/* Give the decomposition in scope , basic identifier
				 * Assume that identifier is scoped
				 */
				static std::pair<std::string, std::string> decompose(const std::string&);

				/*
				 * Return "" if there is no scope
				 */
				static std::string get_scope(const std::string&);

				/*
				 * Get basic identifier (without scope)
				 */
				static std::string get_identifier(const std::string&);
		};
	};
};

#endif /* _COMPILER_SCOPE_HH_ */
