#include <network/msg.hh>

#include <boost/algorithm/string/trim.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/variant.hpp> 
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

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

		template <class Archive>
		void serialize(Archive & ar, hyper::logic::expression& e, const unsigned int version)
		{
			(void) version;
			ar & e.expr;
		}

		template <class Archive>
		void serialize(Archive & ar, hyper::logic::function_call& f, const unsigned int version)
		{
			(void) version;
			ar & f.name & f.args;
		}

		template <class Archive>
		void serialize(Archive& ar, hyper::logic::empty& e, const unsigned int version)
		{
			(void) version; (void) e; (void) ar;
		}

		template <typename Archive, typename T>
		void serialize(Archive& ar, hyper::logic::Constant<T>& c, const unsigned int version)
		{
			(void) version; 
			ar & c.value;
		}
	}
}

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

#define REGISTER_SERIALIZE(name) \
	    template void name::serialize<boost::archive::text_iarchive>( \
                    boost::archive::text_iarchive & ar, const unsigned int file_version); \
	    template void name::serialize<boost::archive::text_oarchive>( \
                    boost::archive::text_oarchive & ar, const unsigned int file_version); \
	    template void name::serialize<boost::archive::binary_iarchive>( \
                    boost::archive::binary_iarchive & ar, const unsigned int file_version); \
	    template void name::serialize<boost::archive::binary_oarchive>( \
                    boost::archive::binary_oarchive & ar, const unsigned int file_version); 

namespace hyper {
	namespace network {
		template<class Archive>
		void request_name::serialize(Archive & ar, const unsigned int version)
		{
			(void) version;
			ar & name;
		}

		REGISTER_SERIALIZE(request_name)

		template<class Archive>
		void request_name_answer::serialize(Archive & ar, const unsigned int version)
		{
			(void) version;
			ar & name & success & endpoints;
		}

		REGISTER_SERIALIZE(request_name_answer)

		template<class Archive>
		void register_name::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & name & endpoints;
		}

		REGISTER_SERIALIZE(register_name)

		template<class Archive>
		void register_name_answer::serialize(Archive & ar, const unsigned int version) 
		{
			(void) version;
			ar & name & success;
		}

		REGISTER_SERIALIZE(register_name_answer)

		template<class Archive>
		void ping::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & name & value;
		}

		REGISTER_SERIALIZE(ping)

		template<class Archive>
		void request_list_agents::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & id & src;
		}

		REGISTER_SERIALIZE(request_list_agents)

		template<class Archive>
		void list_agents::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & id & src & all_agents;
		}

		REGISTER_SERIALIZE(list_agents)

		template<class Archive>
		void inform_new_agent::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & new_agents;
		}

		REGISTER_SERIALIZE(inform_new_agent)

		template<class Archive>
		void inform_death_agent::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & dead_agents;
		}

		REGISTER_SERIALIZE(inform_death_agent)

		template<class Archive>
		void request_variable_value::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & id & src & var_name;
		}

		REGISTER_SERIALIZE(request_variable_value)

		template<class Archive>
		void variable_value::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & id & src & var_name & success & value ;
		}

		REGISTER_SERIALIZE(variable_value)

		template <class Archive>
		void request_constraint::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & id & src & unify_list & constraint & err_ctx & repeat & delay; 
		}

		REGISTER_SERIALIZE(request_constraint)

		template <class Archive>
		void request_constraint2::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & id & src & unify_list & constraint & err_ctx & repeat & delay; 
		}

		REGISTER_SERIALIZE(request_constraint2)

		template <class Archive>
		void request_constraint_ack::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & id & src & acked; 
		}

		REGISTER_SERIALIZE(request_constraint_ack)

		template <class Archive>
		void request_constraint_answer::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & id & src & state & err_ctx;
		}

		REGISTER_SERIALIZE(request_constraint_answer)

		template <class Archive>
		void log_msg::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & date & src & msg;
		}

		log_msg::log_msg(const std::string& src_, const std::string& msg_) :
			date(boost::posix_time::microsec_clock::local_time()),
			src(src_), msg(msg_)
		{
			boost::trim(msg);
		}

		REGISTER_SERIALIZE(log_msg)

		template <class Archive>
		void terminate::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & reason;
		}

		REGISTER_SERIALIZE(terminate)

		template <class Archive>
		void abort::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & src & id;
		}

		REGISTER_SERIALIZE(abort)

		template <class Archive>
		void pause::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & src & id;
		}

		REGISTER_SERIALIZE(pause)

		template <class Archive>
		void resume::serialize(Archive& ar, const unsigned int version)
		{
			(void) version;
			ar & src & id;
		}

		REGISTER_SERIALIZE(resume)


#if 0
		std::ostream& operator << (std::ostream& oss, const message_variant& m) 
		{
			oss << boost::apply_visitor(message_print(), m);
			return oss;
		}
#endif
	}
}


