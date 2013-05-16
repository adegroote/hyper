#include <iostream>

#include <boost/variant/apply_visitor.hpp>

#include <network/runtime_error.hh>

namespace {
	struct print : public boost::static_visitor<void>
	{
		std::ostream& oss;

		print(std::ostream& oss) : oss(oss) {}

		void operator() (const hyper::network::success& ) const {
			oss << "<success>";
		}

		void operator() (const hyper::network::assertion_failure& f) const {
			oss << "<assertion_failure " << f.what << " > ";
		}

		void operator() (const hyper::network::constraint_failure & f) const {
			oss << "<constraint_failure " << f.what << " > ";
		}

		void operator() (const hyper::network::read_failure & f) const {
			oss << "<read_failure " << f.symbol << " > ";
		}

		void operator() (const hyper::network::execution_failure& f) const {
			oss << "<execution_failure " << f.what << " >";
		}
	};
}

namespace hyper {
	namespace network {
		std::ostream& operator << (std::ostream& oss, const runtime_failure& r)
		{
			boost::apply_visitor(print(oss), r.error);
			return oss;
		}
	}
}
