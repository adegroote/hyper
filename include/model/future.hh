#ifndef HYPER_MODEL_FUTURE_HH_
#define HYPER_MODEL_FUTURE_HH_

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace hyper {
	namespace model {

		struct ability_test;

		template <typename T>
		class future_value {
			private:
				struct future_value_inner {
					bool is_computing;
					std::string name;
					boost::mutex mtx;
					boost::condition_variable cond;
					boost::optional<T> res;
				};

				boost::shared_ptr<future_value_inner> ptr;

				boost::optional<T>& get_raw() { return ptr->res; }

				friend struct ability_test;

			public:
				// XXX to please ruby wrapper
				future_value() :
					ptr(boost::make_shared<future_value_inner>())
				{}

				explicit future_value(const std::string& name):
					ptr(boost::make_shared<future_value_inner>())
				{
					ptr->is_computing = true;
					ptr->name = name;
				}

				const std::string& name() const { return ptr->name; };


				T get() {
					boost::unique_lock<boost::mutex> lock(ptr->mtx);
					while( ptr->is_computing )
						ptr->cond.wait(lock);

					if (ptr->res) 
						return T(); // XXX Not a nice way to signal it 
					else 
						return *(ptr->res);
				}

				void signal_ready() {
					boost::unique_lock<boost::mutex> lock(ptr->mtx);
					ptr->is_computing = false;
					ptr->cond.notify_one();
				}
		};
	}
}

#endif /* HYPER_MODEL_FUTURE_HH_ */
