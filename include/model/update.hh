#ifndef HYPER_MODEL_UPDATE_HH_
#define HYPER_MODEL_UPDATE_HH_

#include <map>
#include <string>

#include <network/msg.hh>

#include <boost/function.hpp>
#include <boost/system/error_code.hpp>

namespace hyper {
	namespace model {
		struct ability;

		class updater {
			public:
			typedef boost::function<void (const boost::system::error_code&)> cb_type;

			private:
			typedef boost::function<void (network::identifier, const std::string&, cb_type)> 
				fun_type;
			typedef std::map<std::string, fun_type> map_type;
			map_type map;
			ability & a;

			public:
				updater(ability& a_);

				template <typename T>
				bool add(const std::string& var, const std::string& remote_var, T& value);

				bool add(const std::string& var, const std::string& task);
				
				bool add(const std::string& var);

				void async_update(const std::string& var, network::identifier id, 
								  const std::string& str, cb_type cb);
		};
	}
}

#endif /* HYPER_MODEL_UPDATE_HH_ */
