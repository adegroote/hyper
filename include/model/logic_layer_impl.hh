#ifndef HYPER_MODEL_LOGIC_LAYER_IMPL
#define HYPER_MODEL_LOGIC_LAYER_IMPL

#include <model/logic_layer.hh>
#include <model/ability.hh>
#include <model/operator.hh>

#include <boost/make_shared.hpp>

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

		template <typename Func>
		void logic_layer::add_predicate(const std::string& s)
		{
			size_t size_args = boost::mpl::size<typename Func::args_type>::type::value;
			a_.f_map.add(s, new function_execution<Func>());
			execFuncs.add(s, size_args, 0);
			engine.add_predicate(s, size_args, 0);
		}

		template <typename Func>
		void logic_layer::add_func(const std::string& s)
		{
			size_t size_args = boost::mpl::size<typename Func::args_type>::type::value;
			a_.f_map.add(s, new function_execution<Func>());
			execFuncs.add(s, size_args, 0);
			engine.add_func(s, size_args);
		}
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_IMPL */
