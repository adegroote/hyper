#ifndef _NETWORK_MSG_HH_
#define _NETWORK_MSG_HH_

#include <stdint.h>

#include <iostream>
#include <string>

#include <logic/expression.hh>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/mpl/vector/vector20.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/variant.hpp> 
#include <boost/serialization/split_member.hpp>

#include <boost/variant.hpp>

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
				void serialize(Archive & ar, const unsigned int version)
				{
					(void) version;
					ar & name;
				}

			public:
				std::string name;
		};

		struct register_name_answer
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
					ar & success;

					if (success) {
						addr = endpoint.address().to_string();
						port = endpoint.port();

						ar & addr;
						ar & port;
					}
				}

				template<class Archive>
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

		struct ping
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & name & value;
				}
			public:
				std::string name;
				uint64_t value;
		};

		struct pong
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & value;
				}
			public:
				uint64_t value;
		};

		struct request_variable_value
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & var_name;
				}
			public:
				std::string var_name;
		};

		struct variable_value
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & var_name & success & value ;
				}
			public:
				std::string var_name;
				bool success;
				std::string value;
		};

		struct request_constraint
		{
			private:
				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & constraint;
				}
			public:
				std::string constraint;
		};

		struct request_constraint_ack
		{
			private:
				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & acked & id;
				}
			public:
				bool acked;
				size_t id;
		};

		struct request_constraint_answer
		{
			private:
				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & success & id;
				}
			public:
				bool success;
				size_t id;
		};

		struct log_msg
		{
			private:
				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & date & src & msg;
				}

			public:
				boost::posix_time::ptime date;
				std::string src;
				std::string msg;

				log_msg() {}
				log_msg(const std::string& src_, const std::string& msg_) :
					date(boost::posix_time::microsec_clock::local_time()),
					src(src_), msg(msg_)
				{
					boost::trim(msg);
				}
		};


		typedef boost::mpl::vector12<
			request_name,
			request_name_answer,
			register_name,
			register_name_answer,
			ping,
			pong,
			request_variable_value,
			variable_value,
			request_constraint,
			request_constraint_ack,
			request_constraint_answer,
			log_msg
		> message_types;

#define MESSAGE_TYPE_MAX 12

		typedef boost::make_variant_over<message_types>::type message_variant;

		std::ostream& operator << (std::ostream&, const message_variant&);
	}
}

#endif /* _NETWORK_MSG_HH_ */
