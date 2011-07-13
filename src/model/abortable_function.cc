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

		void abortable_computation::terminaison(const boost::system::error_code& e)
		{
			std::vector<bool> b;
			std::transform(seq.begin(), seq.end(), std::back_inserter(b),
						   boost::bind(&abortable_function_base::abort, _1));

			wait_terminaison = true;
			err = e;
			still_pending = std::count(b.begin(), b.end(), true);

			if (!still_pending) cb(e);
			
		}

		bool abortable_computation::check_is_terminated(const boost::system::error_code& e)
		{
			if (!wait_terminaison)
				return false;

			if (e == boost::system::errc::interrupted) {
				still_pending--;
				if (!still_pending)
					cb(err);
				return true;
			}

			return false;

		}

		void abortable_computation::handle_computation(const boost::system::error_code& e)
		{
			if (check_is_terminated(e)) return;

			if (e) {
				terminaison(e);
			} else {
				if (user_ask_abort) 
					return terminaison(make_error_code(exec_layer_error::interrupted));
				if (index+1 == seq.size())
					terminaison(boost::system::error_code());
				else {
					index++;
					seq[index]->compute(boost::bind(
								&abortable_computation::handle_computation,
								this, boost::asio::placeholders::error));
				}
			}
		}

		void abortable_computation::compute(cb_type cb_) 
		{
			cb = cb_;
			index = 0;
			user_ask_abort = false;
			wait_terminaison = false;

			seq[index]->compute(boost::bind(&abortable_computation::handle_computation,
						this, boost::asio::placeholders::error));
		}

		void abortable_computation::abort() 
		{
			seq[index]->abort();
			user_ask_abort = true;
		}
	}
}
