#ifndef _NETWORK_MSG_HH_
#define _NETWORK_MSG_HH_

#include <stdint.h>

#include <string>

#include <boost/asio.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/serialization/serialization.hpp>

namespace hyper {
	namespace network {

		struct header
		{
			uint32_t type;
			uint32_t size;
		};

		struct request_name
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive & ar, const unsigned int version)
				{
					(void) version;
					ar & name;
				}

			public:
				std::string name;
		};

		struct request_name_answer
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void save(Archive & ar, const unsigned int version) const
				{
					(void) version;
					unsigned short port;
					std::string addr;
					ar & name;
					ar & success;
					if (success) {
						addr = endpoint.address().to_string();
						port =  endpoint.port();

						ar & addr;
						ar & port;
					}
				}

				template <class Archive>
				void load(Archive & ar, const unsigned int version)
				{
					(void) version;
					ar & name;
					ar & success;
					if (success) {
						std::string addr;
						boost::asio::ip::address address;
						unsigned short port;

						ar & addr;
						ar & port;

						address = boost::asio::ip::address::from_string(addr);
						endpoint = boost::asio::ip::tcp::endpoint(address, port);
					}
				}

				BOOST_SERIALIZATION_SPLIT_MEMBER()

			public:
				std::string name;
				bool success;
				boost::asio::ip::tcp::endpoint endpoint;
		};

		struct register_name
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void save(Archive & ar, const unsigned int version) const
				{
					(void) version;
					std::string addr;
					unsigned short port;
					ar & name;

					addr = endpoint.address().to_string();
					port = endpoint.port();

					ar & addr;
					ar & port;
				}

				template<class Archive>
				void load(Archive & ar, const unsigned int version)
				{
					(void) version;
					ar & name;

					std::string addr;
					boost::asio::ip::address address;
					unsigned short port;

					ar & addr;
					ar & port;

					address = boost::asio::ip::address::from_string(addr);
					endpoint = boost::asio::ip::tcp::endpoint(address, port);
				}

				BOOST_SERIALIZATION_SPLIT_MEMBER()
			public:
				std::string name;
				boost::asio::ip::tcp::endpoint endpoint;
		};

		struct register_name_answer
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive & ar, const unsigned int version)
				{
					(void) version;
					ar & name;
					ar & success;
				}
			public:
				std::string name;
				bool success;
		};

		typedef boost::mpl::vector<
			request_name,
			request_name_answer,
			register_name,
			register_name_answer
		> message_types;

#define MESSAGE_TYPE_MAX	20

	};
};

#endif /* _NETWORK_MSG_HH_ */
