
#ifndef _COMPILER_ABILITY_HH
#define _COMPILER_ABILITY_HH

#include <hyperConfig.hh>

#include <compiler/depends.hh>
#include <compiler/symbols.hh>
#include <compiler/task.hh>

#include <list>
#include <set>

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
			symbolACL(const symbol& s_) : s(s_), acl(CONTROLABLE) {};
			symbolACL(const symbol& s_, abilityACL acl_) : s(s_), acl(acl_) {};
		};

		class universe;
		class typeList;

		class ability : public boost::noncopyable {
			private:
				symbolList controlable_list;
				symbolList readable_list;
				symbolList private_list;

				std::string name_;

				std::list<task> tasks;
				typedef std::list<task>::const_iterator task_const_iterator;

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


				/*
				 * Add a task to the ability
				 * @returns false if a task with the same name already exists.
				 * In this case, the ability is not modified
				 * @returns true otherwise (and the task is really added to @tasks
				 */
				bool add_task(const task& t);

				const task& get_task(const std::string& name) const;
				task& get_task(const std::string& name);

				task_const_iterator task_begin() const { return tasks.begin(); }
				task_const_iterator task_end() const { return tasks.end(); }

				const std::string name() const
				{
					return name_;
				}

				void dump_include(std::ostream& oss, const universe& u) const;
				void dump(std::ostream& oss, const universe& u) const;
				void agent_export_declaration(std::ostream& oss, const typeList& tList) const;
				void agent_export_implementation(std::ostream& oss, const typeList& tList) const;
				void dump_swig_ability_types(std::ostream& oss, const typeList& tList) const;

				depends get_function_depends(const universe& u) const;
				std::set<std::string> get_type_depends(const typeList& tList) const;
		};
	}
}


#endif /* _COMPILER_ABILITY_HH */
