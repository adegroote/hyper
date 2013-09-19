#ifndef HYPER_MODEL_UPDATE_HH_
#define HYPER_MODEL_UPDATE_HH_

#include <map>
#include <string>
#include <vector>

#include <network/types.hh>
#include <network/runtime_error.hh>

#include <boost/array.hpp>
#include <boost/function/function1.hpp>
#include <boost/function/function3.hpp>
#include <boost/system/error_code.hpp>

namespace hyper {
	namespace model {
		struct ability;

		struct local_var {
			std::string var;
			bool is_updated;

			local_var() {}
			local_var(const std::string& var) : var(var), is_updated(false) {}
		};

		struct local_vars {
			std::vector<local_var> vars;

			local_vars() {}

			template <size_t N>
			local_vars(const boost::array<std::string, N>& vars_) 
			{
				std::copy(vars_.begin(), vars_.end(), std::back_inserter(vars));
			}

			bool is_terminated() const
			{
				bool res = true;
				for (size_t i = 0; i < vars.size(); ++i)
					res &= vars[i].is_updated;
				return res;
			}

			void reset()
			{
				for (size_t i = 0; i < vars.size(); ++i)
					vars[i].is_updated = false;
			}

			network::error_context error() const
			{
				network::error_context err;
				std::vector<local_var>::const_iterator it;
				for (it = vars.begin(); it != vars.end(); ++it)
				{
					if (!it->is_updated) {
						network::runtime_failure fail(
								network::read_failure(it->var));

						err.push_back(fail);
					}
				}
				return err;
			}
		};

		class updater {
			public:
			typedef boost::function<void (const boost::system::error_code&,
										  const hyper::network::error_context&)> cb_type;

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

				bool remove(const std::string& var);

				void async_update(const std::string& var, network::identifier id, 
								  const std::string& str, cb_type cb);

				void async_update(local_vars& vars, network::identifier id,
								  const std::string& str, cb_type cb);

		};

		template <typename A, size_t N, typename vectorT>
		class update_variables;
	}
}

#endif /* HYPER_MODEL_UPDATE_HH_ */
