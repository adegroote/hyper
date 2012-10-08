#ifndef HYPER_NETWORK_SELECT_SERIALIZATION_HH
#define HYPER_NETWORK_SELECT_SERIALIZATION_HH

#include <hyperConfig.hh>

#if HYPER_SERIALIZATION == TEXT_ARCHIVE
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#define HYPER_INPUT_ARCHIVE boost::archive::text_iarchive
#define HYPER_OUTPUT_ARCHIVE boost::archive::text_oarchive
#elif HYPER_SERIALIZATION == BINARY_ARCHIVE
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#define HYPER_INPUT_ARCHIVE boost::archive::binary_iarchive
#define HYPER_OUTPUT_ARCHIVE boost::archive::binary_oarchive
#else
#error "Unknown serialization kind"
#endif



#endif /* HYPER_NETWORK_SELECT_SERIALIZATION_HH */
