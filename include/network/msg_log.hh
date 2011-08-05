#ifndef HYPER_NETWORK_MSG_LOG_HH_
#define HYPER_NETWORK_MSG_LOG_HH_

#include <string>
#include <boost/date_time/posix_time/ptime.hpp>

namespace hyper {
	namespace network {
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
	}
}

#endif /* HYPER_NETWORK_MSG_LOG_HH_ */
