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
			hyper::model::add_equalable_type<T>(a_.f_map, s);
			hyper::model::add_logic_equalable_type(engine, s);
		}

		template <typename T>
		void logic_layer::add_numeric_type(std::string s)
		{
			hyper::model::add_numeric_type<T>(a_.f_map, s);
			hyper::model::add_logic_numeric_type(engine, s);
		}

		template <typename T>
		void logic_layer::add_comparable_type(std::string s)
		{
			hyper::model::add_comparable_type<T>(a_.f_map, s);
			hyper::model::add_logic_comparable_type(engine, s);
		}

		template <typename Func>
		void logic_layer::add_predicate(const std::string& s, 
				const std::vector<std::string>& args)
		{
			size_t size_args = boost::mpl::size<typename Func::args_type>::type::value;
			a_.f_map.add(s, new function_execution<Func>());
			engine.add_predicate(s, size_args, args, 0);
		}

		template <typename Func>
		void logic_layer::add_func(const std::string& s, const std::vector<std::string>& args)
		{
			size_t size_args = boost::mpl::size<typename Func::args_type>::type::value;
			a_.f_map.add(s, new function_execution<Func>());
			engine.add_func(s, size_args, args);
		}
	}
}

#endif /* HYPER_MODEL_LOGIC_LAYER_IMPL */
