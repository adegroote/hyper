#ifndef HYPER_MODEL_TYPE_HH_
#define HYPER_MODEL_TYPE_HH_

#include <string>
#include <network/types.hh>

namespace hyper { namespace model {
	struct identifier {
		std::string first;
		hyper::network::identifier second;

		template<class Archive>
		void serialize(Archive & ar, const unsigned int version);
	};
}}

#endif /* HYPER_MODEL_TYPE_HH_ */
