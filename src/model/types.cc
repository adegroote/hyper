#include <model/types.hh>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/serialization.hpp>

namespace hyper {
	namespace model {
		template<class Archive>
		void identifier::serialize(Archive & ar, const unsigned int version)
		{
			ar & first & second;
		}

template void identifier::serialize<boost::archive::text_iarchive>(
		    boost::archive::text_iarchive & ar, const unsigned int file_version);
template void identifier::serialize<boost::archive::text_oarchive>(
		    boost::archive::text_oarchive & ar,  const unsigned int file_version);
	}
}
