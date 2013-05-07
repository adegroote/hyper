#ifndef _NETWORK_MSG_HH_
#define _NETWORK_MSG_HH_

#include <string>

#include <boost/mpl/vector/vector20.hpp>

#include <network/msg_constraint.hh>
#include <network/msg_log.hh>
#include <network/msg_name.hh>
#include <network/msg_proxy.hh>
#include <network/types.hh>

namespace hyper {
	namespace network {

		struct header
		{
			uint32_t type;
			uint32_t size;
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

		struct terminate
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			std::string reason;

			terminate() {};
			terminate(const std::string& src) : reason(src) {}
		};

		typedef boost::mpl::vector19<
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
			request_constraint2,
			request_constraint_answer,
			log_msg,
			terminate,
			abort, 
			pause,
			resume
		> message_types;

#define MESSAGE_TYPE_MAX 19

	}
}

#endif /* _NETWORK_MSG_HH_ */
