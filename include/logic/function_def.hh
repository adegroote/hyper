#ifndef _HYPER_LOGIC_FUNCTION_HH_
#define _HYPER_LOGIC_FUNCTION_HH_

#include <cassert>
#include <map>
#include <string>
#include <vector>

#include <boost/optional/optional_fwd.hpp>
#include <boost/noncopyable.hpp>

namespace hyper {
	namespace logic {
		typedef std::size_t functionId ;

		struct eval_predicate;

		/*
		 * In the logic world, everything is untyped
		 * (type validation has been done in a previous step, so we don't care)
		 * eval is not owned by function_def, but by the funcDefList associated
		 */
		struct function_def 
		{
			std::string name;
			size_t arity;
			eval_predicate* eval_pred;
			bool unify_predicate;

			function_def() {};
			function_def(const std::string& name_, const size_t arity_, 
						 eval_predicate* eval_ = 0, bool unify_predicate = false):
				name(name_), arity(arity_), eval_pred(eval_),
				unify_predicate(unify_predicate) {}
		};

		/*
		 * Each function is identified by a functionId, for future search
		 *
		 * This id won't be unique between the different abilities, so won't be
		 * used to encode the msg exchanged, but will be used to compute logic
		 * search.
		 *
		 * funcDefList is also responsible of the lifetime of the
		 * eval_predicate*, stored in list_eval
		 */
		class funcDefList : public boost::noncopyable
		{
			private:
				typedef std::vector<function_def> funcV;
				typedef std::vector<eval_predicate*> funcE;
				typedef std::map<std::string, functionId> funcM;

				funcV list;
				funcE list_eval;
				funcM m;

			public:
				funcDefList();
				~funcDefList();

				functionId add(const std::string& name, size_t arity, eval_predicate* eval = 0,
							   bool unify_predicate = false);

				const function_def& get(functionId id) const
				{
					assert (id < list.size());
					return list[id];
				}

				boost::optional<function_def> get(const std::string& name) const;
				boost::optional<functionId> getId(const std::string& name) const;

				size_t size() const { return list.size(); }
		};
	}
}
#endif /* _HYPER_LOGIC_FUNCTION_HH_ */
