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
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>

#include <boost/variant.hpp>

namespace boost {
	namespace serialization {
		template<class Archive>
		void save(Archive& ar, const boost::asio::ip::tcp::endpoint& e, const unsigned int version)
		{
			(void) version;
			std::string addr = e.address().to_string();
			unsigned short port = e.port();
			ar & addr & port;
		}

		template<class Archive>
		void load(Archive& ar, boost::asio::ip::tcp::endpoint& e, const unsigned int version)
		{
			(void) version;
			std::string addr;
			boost::asio::ip::address address;
			unsigned short port;

			ar & addr & port;

			address = boost::asio::ip::address::from_string(addr);
			e = boost::asio::ip::tcp::endpoint(address, port);
		}

		template<class Archive>
		void serialize(Archive & ar, boost::asio::ip::tcp::endpoint& e, const unsigned int version)
		{
			split_free(ar, e, version); 
		}
	}
}

namespace hyper {
	namespace network {

		struct header
		{
			uint32_t type;
			uint32_t size;
		};

		typedef size_t identifier;

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
				void serialize(Archive & ar, const unsigned int version)
				{
					(void) version;
					ar & name & success & endpoints;
				}

			public:
				std::string name;
				bool success;
				std::vector<boost::asio::ip::tcp::endpoint> endpoints;
		};

		struct register_name
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & name & endpoints;
				}

			public:
				std::string name;
				std::vector<boost::asio::ip::tcp::endpoint> endpoints;
		};

		struct register_name_answer
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive & ar, const unsigned int version) 
				{
					(void) version;
					ar & name & success;
				}

			public:
				std::string name;
				bool success;
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
				identifier value;
		};

		struct request_list_agents
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & id & src;
				}
			public:
				mutable identifier id;
				mutable std::string src;
		};

		struct list_agents 
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & id & src & all_agents;
				}
			public:
				mutable identifier id;
				mutable std::string src;
				std::vector<std::string> all_agents;
		};

		struct inform_new_agent 
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & new_agents;
				}
			public:
				std::vector<std::string> new_agents;
		};

		struct inform_death_agent
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & dead_agents;
				}
			public:
				std::vector<std::string> dead_agents;
		};

		struct request_variable_value
		{
			private:
				friend class boost::serialization::access;
				template<class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & id & src & var_name;
				}
			public:
				mutable identifier id;
				mutable std::string src;
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
					ar & id & src & var_name & success & value ;
				}
			public:
				mutable identifier id;
				mutable std::string src;
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
					ar & id & src & constraint;
				}
			public:
				mutable identifier id;
				mutable std::string src;
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
					ar & id & src & acked; 
				}
			public:
				mutable identifier id;
				mutable std::string src;
				bool acked;
		};

		struct request_constraint_answer
		{
			private:
				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & id & src & success;
				}
			public:
				mutable identifier id;
				mutable std::string src;
				bool success;
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

		struct terminate
		{
			private:
				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & reason;
				}
			public:
				std::string reason;

				terminate() {};
				terminate(const std::string& src) : reason(src) {}
		};

		struct abort 
		{
			private:
				friend class boost::serialization::access;
				template <class Archive>
				void serialize(Archive& ar, const unsigned int version)
				{
					(void) version;
					ar & id;
				}
			public:
				identifier id;

				abort() {};
				abort(identifier id) : id(id) {}
		};

		typedef boost::mpl::vector17<
			request_name,
			request_name_answer,
			register_name,
			register_name_answer,
			ping,
			request_list_agents,
			list_agents,
			inform_new_agent,
			inform_death_agent,
			request_variable_value,
			variable_value,
			request_constraint,
			request_constraint_ack,
			request_constraint_answer,
			log_msg,
			terminate,
			abort
		> message_types;

#define MESSAGE_TYPE_MAX 17

		typedef boost::make_variant_over<message_types>::type message_variant;

		std::ostream& operator << (std::ostream&, const message_variant&);
	}
}

#endif /* _NETWORK_MSG_HH_ */
