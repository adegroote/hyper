#ifndef HYPER_MODEL_LOGIC_LAYER_IMPL
#define HYPER_MODEL_LOGIC_LAYER_IMPL

#include <model/logic_layer.hh>
#include <model/ability.hh>
#include <model/operator.hh>

namespace hyper {
	namespace model {
		template <typename T>
		void logic_layer::add_equalable_type(std::string s) 
		{
			hyper::model::add_equalable_type<T>(a_.f_map, execFuncs, s);
		}

		template <typename T>
		void logic_layer::add_numeric_type(std::string s)
		{
			hyper::model::add_numeric_type<T>(a_.f_map, execFuncs, s);
		}

		template <typename T>
		void logic_layer::add_comparable_type(std::string s)
		{
			hyper::model::add_comparable_type<T>(a_.f_map, execFuncs, s);
		}
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_IMPL */
