#include <boost/thread/locks.hpp>

#include <network/nameserver.hh>

namespace hyper {
	namespace network {
		namespace ns {
			map_addr::map_addr() : map_(), m_()  {};

			bool map_addr::add(const std::string& key, const addr_storage& s)
			{
				boost::upgrade_lock<boost::shared_mutex> lock(m_);
				addrs::const_iterator it = map_.find(key);
				if (it != map_.end())
					return false;

				map_[key] = s;
				return true;
			}

			bool map_addr::remove(const std::string& key)
			{
				boost::upgrade_lock<boost::shared_mutex> lock(m_);
				addrs::iterator it = map_.find(key);
				if (it == map_.end())
					return false;

				map_.erase(it);
				return true;
			}

			bool map_addr::isIn(const std::string& key) const
			{
				boost::shared_lock<boost::shared_mutex> lock(m_);
				addrs::const_iterator it = map_.find(key);
				return (it != map_.end());
			}

			std::pair<bool, addr_storage>
			map_addr::get(const std::string& key) const
			{
				boost::shared_lock<boost::shared_mutex> lock(m_);
				addrs::const_iterator it = map_.find(key);
				if (it == map_.end())
					return std::make_pair(false, addr_storage());

				return std::make_pair(true, it->second);
			}
		};

		namespace tcp {
			ns_visitor::ns_visitor(ns::map_addr& map) : map_(map) {};

			ns::output_variant ns_visitor::operator() (const request_name& r) const
			{
				std::pair<bool, ns::addr_storage> res;
				res = map_.get(r.name);

				request_name_answer res_msg;
				res_msg.name = r.name;
				res_msg.success = res.first;
				res_msg.endpoint = res.second.tcp_endpoint;

				return res_msg;
			}

			ns::output_variant ns_visitor::operator() (const register_name& r) const
			{
				ns::addr_storage addr;
				addr.tcp_endpoint = r.endpoint;
				bool res = map_.add(r.name, addr);

				register_name_answer res_msg;
				res_msg.name = r.name;
				res_msg.success = res;

				return res_msg;
			}
		};

		name_server::name_server(const std::string& addr, const std::string& port):
			map_(), tcp_ns_(addr, port, tcp::ns_visitor(map_)) {};

		void name_server::run()
		{
			tcp_ns_.run();
		}

		void name_server::stop()
		{
			tcp_ns_.stop();
		}
	};
};
