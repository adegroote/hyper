#include <model/types.hh>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/serialization.hpp>

namespace hyper {
	namespace model {
		template<class Archive>
		void identifier::serialize(Archive & ar, const unsigned int version)
		{
			ar & first & second;
		}

template void identifier::serialize<boost::archive::binary_iarchive>(
		    boost::archive::binary_iarchive & ar, const unsigned int file_version);
template void identifier::serialize<boost::archive::binary_oarchive>(
		    boost::archive::binary_oarchive & ar,  const unsigned int file_version);
	}
}
