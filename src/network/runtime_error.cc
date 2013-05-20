#include <iostream>
#include <algorithm>

#include <boost/variant/apply_visitor.hpp>
#include <boost/bind.hpp>

#include <network/runtime_error.hh>

namespace {
	struct print : public boost::static_visitor<void>
	{
		std::ostream& oss;

		print(std::ostream& oss) : oss(oss) {}

		void operator() (const hyper::network::unknown_error& ) const {
			oss << "<unknown_error>";
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
			oss << "<execution_failure " << f.what;
			if (f.extra_information != "")
				oss << " with " << f.extra_information;
			oss << " >";
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

		void
		runtime_failure::pretty_print(std::ostream& oss, int indent) const
		{
			for (int i; i < indent; i++) oss << '\t';

			oss << error;
			if (recipe_name != "") 
				oss << " in " << recipe_name << std::endl; 
			else
				oss << std::endl;

			std::for_each(error_cause.begin(), error_cause.end(),
						  boost::bind(&runtime_failure::pretty_print,
									  _1, boost::ref(oss), indent + 1));
		}
	}
}
