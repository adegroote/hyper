#ifndef HYPER_NETWORK_MSG_PROXY_HH_
#define HYPER_NETWORK_MSG_PROXY_HH_

#include <string>

#include <network/types.hh>
#include <network/runtime_error.hh>

namespace hyper {
	namespace network {
		struct request_variable_value
		{
			template<class Archive>
			void serialize(Archive& ar, const unsigned int version);

			mutable identifier id;
			mutable std::string src;
			std::string var_name;
		};

		struct variable_value
		{
			template<class Archive>
			void serialize(Archive& ar, const unsigned int version);

			mutable identifier id;
			mutable std::string src;
			std::string var_name;
			bool success;
			std::string value;
			error_context err_ctx;
		};

	}
}

#endif /* HYPER_NETWORK_MSG_PROXY_HH_ */
