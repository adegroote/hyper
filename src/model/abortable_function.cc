#include <model/abortable_function.hh>

namespace hyper {
	namespace model {
		const char* exec_layer_category_impl::name() const {
			return "exec_layer";
		}

		std::string exec_layer_category_impl::message(int ev) const
		{
			switch(ev) {
				case exec_layer_error::ok :
					return "ok";
				case exec_layer_error::interrupted:
					return "interrupted";
				case exec_layer_error::execution_failed:
					return "execution_failed";
				case exec_layer_error::execution_ko:
					return "execution_ok";
				default:
					return "unknow_error";
			}
		}

		const boost::system::error_category& exec_layer_category()
		{
			static exec_layer_category_impl instance;
			return instance;
		}
	}
}
