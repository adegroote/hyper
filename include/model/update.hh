#ifndef HYPER_MODEL_UPDATE_HH_
#define HYPER_MODEL_UPDATE_HH_o

#include <map>
#include <string>

#include <boost/function.hpp>
#include <boost/system/error_code.hpp>

namespace hyper {
	namespace model {
		struct ability;

		class updater {
			public:
			typedef boost::function<void (const boost::system::error_code&)> cb_type;

			private:
			typedef boost::function<void (cb_type)> fun_type;
			typedef std::map<std::string, fun_type> map_type;
			map_type map;
			ability & a;

			public:
				updater(ability& a_);

				template <typename T>
				bool add(const std::string& var, const std::string& remote_var, T& value);

				bool add(const std::string& var, const std::string& task);
				
				bool add(const std::string& var);

				void async_update(const std::string& var, cb_type cb);
		};
	}
}

#endif /* HYPER_MODEL_UPDATE_HH_ */
