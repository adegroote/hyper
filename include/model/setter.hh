#ifndef HYPER_MODEL_SETTER_HH_
#define HYPER_MODEL_SETTER_HH_

#include <map>
#include <string>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <logic/expression.hh>

namespace hyper {
	namespace model {

		class ability;

		class setter {
			public:
				typedef boost::function<void (const boost::system::error_code&) > cb_fun;
				typedef boost::function<void (const logic::expression&, cb_fun)> fun;
				typedef std::map<std::string, fun> map_fun;

			private:
				model::ability& a;
				map_fun m;

			public:
				setter(model::ability& a) : a(a) {};
				template <typename T>
				void add(const std::string& s, T& var);
				void set(const std::string& s, const logic::expression& e, cb_fun cb) const;
		};
	}
}

#endif /* HYPER_MODEL_SETTER_HH_ */
