#ifndef HYPER_NETWORK_MESSAGE_NAME_HH_
#define HYPER_NETWORK_MESSAGE_NAME_HH_

#include <string>
#include <vector>

#include <boost/asio/ip/tcp.hpp>

namespace hyper {
	namespace network {
		struct request_name
		{
			std::string name;

			template<class Archive>
			void serialize(Archive & ar, const unsigned int version);
		};

		struct request_name_answer
		{
			template<class Archive>
			void serialize(Archive & ar, const unsigned int version);

			std::string name;
			bool success;
			std::vector<boost::asio::ip::tcp::endpoint> endpoints;
		};

		struct register_name
		{
			template<class Archive>
			void serialize(Archive& ar, const unsigned int version);

			std::string name;
			std::vector<boost::asio::ip::tcp::endpoint> endpoints;
		};

		struct register_name_answer
		{
			template<class Archive>
			void serialize(Archive & ar, const unsigned int version);

			std::string name;
			bool success;
		};
	}
}

#endif /* HYPER_NETWORK_MESSAGE_NAME_HH_ */
