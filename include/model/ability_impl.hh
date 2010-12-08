#ifndef HYPER_MODEL_ABILITY_IMPL_HH_
#define HYPER_MODEL_ABILITY_IMPL_HH_

#include <compiler/scope.hh>
#include <model/ability.hh>
#include <model/update_impl.hh>

namespace hyper {
	namespace model {
		template <typename T>
		void ability::export_variable(const std::string& name, const T& value)
		{
			export_variable_helper(name, value);
			updater.add(name);
		}

		template <typename T>
		void ability::export_variable(const std::string& name, const T& value,
									  const std::string& update)
		{
			export_variable_helper(name, value);
			if (compiler::scope::is_scoped_identifier(update))
				updater.add(name, update, value);
			else
				updater.add(name, update);
		}
	}
}


#endif /* HYPER_MODEL_ABILITY_IMPL_HH_ */
