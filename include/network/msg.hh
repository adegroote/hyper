#ifndef _NETWORK_MSG_HH_
#define _NETWORK_MSG_HH_

#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/mpl/vector/vector20.hpp>

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

		struct ping
		{
			template<class Archive>
			void serialize(Archive& ar, const unsigned int version);

			std::string name;
			identifier value;
		};

		struct request_list_agents
		{
			template<class Archive>
			void serialize(Archive& ar, const unsigned int version);

			mutable identifier id;
			mutable std::string src;
		};

		struct list_agents 
		{
			template<class Archive>
			void serialize(Archive& ar, const unsigned int version);

			mutable identifier id;
			mutable std::string src;
			std::vector<std::string> all_agents;
		};

		struct inform_new_agent 
		{
			template<class Archive>
			void serialize(Archive& ar, const unsigned int version);

			std::vector<std::string> new_agents;
		};

		struct inform_death_agent
		{
			template<class Archive>
			void serialize(Archive& ar, const unsigned int version);

			std::vector<std::string> dead_agents;
		};

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
		};

		struct request_constraint
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			typedef std::pair<std::string, std::string> unification_pair;
			typedef std::vector<unification_pair> unification_list;
			mutable identifier id;
			mutable std::string src;
			std::string constraint;
			unification_list  unify_list;
			bool repeat;
		};

		struct request_constraint_ack
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			mutable identifier id;
			mutable std::string src;
			bool acked;
		};

		struct request_constraint_answer
		{
			enum state_ { SUCCESS, FAILURE, INTERRUPTED };

			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			mutable identifier id;
			mutable std::string src;
			state_ state;
		};

		struct log_msg
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			boost::posix_time::ptime date;
			std::string src;
			std::string msg;

			log_msg() {}
			log_msg(const std::string& src_, const std::string& msg_);
		};

		struct terminate
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			std::string reason;

			terminate() {};
			terminate(const std::string& src) : reason(src) {}
		};

		struct abort 
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			std::string src;
			identifier id;

			abort() {};
			abort(const std::string& src, identifier id) : src(src), id(id) {}
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

	}
}

#endif /* _NETWORK_MSG_HH_ */
