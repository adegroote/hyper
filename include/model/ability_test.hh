#ifndef HYPER_MODEL_MODULE_ABILITY_TEST_HH
#define HYPER_MODEL_MODULE_ABILITY_TEST_HH

#include <model/ability.hh>
#include <model/future.hh>


namespace hyper {
	namespace model {
		struct ability_test : public ability {
			std::string target;
			boost::thread thr;
			
			ability_test(const std::string& name_) : 
				ability(name_ +  "_test", DEBUG_ALL), target(name_),
				thr(boost::bind(&ability::run, this))
			{}

			private:
			template <typename T>
			void handle_get_value(const boost::system::error_code&e,
					network::actor::remote_proxy<ability>* proxy,
					future_value<T> future)
			{
				delete proxy;
				if (e) {
					std::cerr << "Failed to get the value of " << future.name() << std::endl;
				} else {
					std::cerr << "Successful get the value of " << future.name() << std::endl;
				}
				future.signal_ready();
			}

			void handle_get_value(const boost::system::error_code&e,
								  network::actor::remote_proxy<ability>* proxy,
								  const std::string& value);

			void handle_send_constraint(const boost::system::error_code& e,
									    network::request_constraint* msg,
										network::request_constraint_answer* ans);

			protected:
			template <typename T>
			future_value<T> get_value(const std::string& value)
			{
				network::actor::remote_proxy<ability>* proxy = 
					new network::actor::remote_proxy<ability> (*this);
			
				future_value<T> res(value);

				void (ability_test::*f) (const boost::system::error_code&,
										 network::actor::remote_proxy<ability>*,
										 future_value<T>) =
					&ability_test::template handle_get_value<T>;

				proxy->async_get(target, res.name(), res.get_raw(),
						boost::bind(f, this,
									boost::asio::placeholders::error,
									proxy, res));

				return res;
			}

			public:
			void send_constraint(const std::string& constraint);
		};
	}
}

#endif /* HYPER_MODEL_MODULE_ABILITY_TEST_HH */
