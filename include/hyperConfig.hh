#ifndef _HYPER_CONFIG_HH_
#define _HYPER_CONFIG_HH_
/*
 * This file contains various definition which must be used everywhere in the project
 * IT MUST BE THE FIRST INCLUDE
 */

/*
 * XXX Whoot crazy not so well documented stuff
 * Increase the max size of possible variant
 * Don't use preprocessed header as they are compiled with default BOOST_MPL_LIMIT_LIST_SIZE
 */
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 30

#endif /* _HYPER_CONFIG_HH_ */
