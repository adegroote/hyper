#ifndef _HYPER_LOGIC_FUNCTION_HH_
#define _HYPER_LOGIC_FUNCTION_HH_

#include <map>
#include <string>
#include <vector>

#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>

namespace hyper {
	namespace logic {
		typedef std::size_t functionId ;

		/*
		 * In the logic world, everything is untyped
		 * (type validation has been done in a previous step, so we don't care)
		 */
		struct function_def 
		{
			std::string name;
			size_t arity;

			function_def() {};
			function_def(const std::string& name_, const size_t arity_):
				name(name_), arity(arity_) {}
		};

		/*
		 * Each function is identified by a functionId, for future search
		 *
		 * This id won't be unique between the different abilities, so won't be
		 * used to encode the msg exchanged, but will be used to compute logic
		 * search.
		 */
		class funcDefList : public boost::noncopyable
		{
			private:
				typedef std::vector<function_def> funcV;
				typedef std::map<std::string, functionId> funcM;

				funcV list;
				funcM m;

			public:
				funcDefList() {};

				functionId add(const std::string& name, size_t arity);
				function_def get(functionId id) const;
				boost::optional<function_def> get(const std::string& name) const;
		};
	}
}
#endif /* _HYPER_LOGIC_FUNCTION_HH_ */
