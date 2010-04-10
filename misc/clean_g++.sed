#! /bin/sed -f

s/boost::tuples::null_type\(, \)\?//g
s/boost::detail::variant::void_\(, \)\?//g
