#include <network/msg.hh>

#include <boost/archive/text_oarchive.hpp>

namespace {
	struct message_print : boost::static_visitor<std::string>
	{
		template <typename T>
		std::string operator() (const T& m) const
		{
			std::ostringstream archive_stream;
			boost::archive::text_oarchive archive(archive_stream);
			archive << m;
			return archive_stream.str();
		}
	};
}

namespace hyper {
	namespace network {
		std::ostream& operator << (std::ostream& oss, const message_variant& m) 
		{
			oss << boost::apply_visitor(message_print(), m);
			return oss;
		}
	}
}
