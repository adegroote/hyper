#ifndef HYPER_MODEL_GET_VALUE
#define HYPER_MODEL_GET_VALUE

#include <string>
#include <iostream>

#include <network/nameserver.hh>
#include <network/proxy.hh>

namespace hyper {
	namespace model {

		struct exception_no_value {
			std::string ability;
			std::string var;

			exception_no_value(const std::string& ability_,
							  const std::string& var_) :
				ability(ability_), var(var_) {}

			const char* what() const { 
				std::ostringstream oss;
				oss << "Can't get the value of " << var;
				oss << " in ability " << ability;
				return oss.str().c_str();
			}

		};

		template <typename T>
		T get_value(const std::string& ability, const std::string& var)
		{
			boost::asio::io_service io_s;
			network::name_client name_(io_s, "localhost", "4242");
			typename network::remote_proxy_sync<T, network::name_client>
			r_proxy(io_s, ability, var, name_);
			const boost::optional<T>& value = 
				r_proxy.get(boost::posix_time::seconds(1));
			if (!value) 
				throw exception_no_value(ability, var);
			return *value;
		}

	}
}

#endif /* HYPER_MODEL_GET_VALUE */
