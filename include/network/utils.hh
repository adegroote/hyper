#ifndef _HYPER_NETWORK_UTILS_HH_
#define _HYPER_NETWORK_UTILS_HH_

#include <boost/mpl/assert.hpp> 
#include <boost/mpl/reverse_fold.hpp> 
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp> 
#include <boost/tuple/tuple.hpp> 
#include <boost/type_traits/is_same.hpp> 

namespace hyper {
	namespace network {

		template <typename Seq>
		struct to_tuple 
		{
			typedef typename boost::mpl::reverse_fold<Seq, boost::tuples::null_type, 
					boost::tuples::cons<boost::mpl::_2, boost::mpl::_1> >::type type; 
		};
	}
}

#endif /* _HYPER_NETWORK_UTILS_HH_ */
