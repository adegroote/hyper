#ifndef HYPER_MODEL_MODULE_ABILITY_TEST_HH
#define HYPER_MODEL_MODULE_ABILITY_TEST_HH

#include <map>

#include <model/ability.hh>
#include <model/future.hh>
#include <model/proxy.hh>
#include <network/log_level.hh>
#include <network/json_archive.hh>
#include <network/msg_constraint.hh>

#include <boost/function/function0.hpp>

namespace hyper {
	namespace model {
		struct ability_test : public ability {
			std::string target;
			remote_proxy proxy;
			typedef std::map<std::string, boost::function<void ()> > getter_map;
			getter_map gmap;
			
			ability_test(const std::string& name_) : 
				ability(name_ +  "_test", DEBUG_ALL), target(name_),
				proxy(*this)
			{}

			private:
			template <typename T>
			void handle_get(const boost::system::error_code&e,
							remote_value<T>& value)
			{
				if (e) {
					std::cerr << "Failed to get the value of " << value.msg.var_name << std::endl;
				} else {
					std::ostringstream oss;
					hyper::network::json_oarchive oa(oss);
					oa << value.value;

					std::cerr << "Get \"" << value.msg.var_name << "\": "<< oss.str() << std::endl;
				}
				this->stop();
			}

			void handle_send_constraint(const boost::system::error_code& e,
										network::identifier id,
										future_value<bool>, 
									    network::request_constraint* msg,
										network::request_constraint_answer* ans);

			future_value<bool> send_constraint(const std::string& constraint, bool repeat);

			template <typename T>
			void get(remote_value<T>& value)
			{
				void (ability_test::*f) (const boost::system::error_code&,
										 remote_value<T>&) =
					&ability_test::template handle_get<T>;

				proxy.async_get(value,
						boost::bind(f, this,
									boost::asio::placeholders::error,
									boost::ref(value)));
			}

			public:
			template <typename T>
			void register_get(const std::string& s, remote_value<T>& t)
			{
				void (ability_test::*f) (remote_value<T>&) = 
					&ability_test::template get<T>;
				gmap[s] = boost::bind(f, this, t);
			}

			int main(int argc, char** argv);
		};
	}
}

#endif /* HYPER_MODEL_MODULE_ABILITY_TEST_HH */
