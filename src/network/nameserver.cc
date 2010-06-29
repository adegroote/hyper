#include <boost/thread/locks.hpp>
#include <boost/lexical_cast.hpp>

#include <network/nameserver.hh>

namespace hyper {
	namespace network {
		namespace ns {
			map_addr::map_addr() : map_(), m_()  {}

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
		}

		namespace tcp {

			ns_port_generator::ns_port_generator(const std::string& port) 
			{
			    using boost::lexical_cast;
				base_port = lexical_cast<unsigned short> (port);
				// increment base_port as it will be used by nameserver
				base_port++;
			}

			unsigned short ns_port_generator::get()
			{
				unsigned short res = base_port;
				if (res == 0) // too bad, we has overflow port max number
					exit(-1); // whoot craps, but nobody cares atm

				base_port++;
				return res;
			}

			ns_visitor::ns_visitor(ns::map_addr& map, ns_port_generator& gen,
								  std::ostream & output) : 
				map_(map), gen_(gen), output_(output) {}

			ns::output_variant ns_visitor::operator() (const request_name& r) const
			{
				std::pair<bool, ns::addr_storage> res;
				res = map_.get(r.name);
				output_ << "receiving name request : " << r << std::endl;

				request_name_answer res_msg;
				res_msg.name = r.name;
				res_msg.success = res.first;
				res_msg.endpoint = res.second.tcp_endpoint;

				output_ << "answering to name request : " << res_msg << std::endl;
				return res_msg;
			}

			ns::output_variant ns_visitor::operator() (const register_name& r) const
			{
				using namespace boost::asio;

				output_ << "receiving name register request : " << r << std::endl;
				ns::addr_storage addr;
				addr.tcp_endpoint = ip::tcp::endpoint(ip::address_v4::any(), gen_.get());
				bool res = map_.add(r.name, addr);

				register_name_answer res_msg;
				res_msg.name = r.name;
				res_msg.endpoint = addr.tcp_endpoint;
				res_msg.success = res;

				output_ << "answering to name register request : " << res_msg << std::endl;
				return res_msg;
			}
		}

		name_server::name_server(const std::string& addr, const std::string& port, 
								 boost::asio::io_service& io_service, bool verbose):
			oss(0), map_(), gen_(port), 
			tcp_ns_(addr, port, 
					tcp::ns_visitor(map_, gen_, 
						verbose ? std::cout : oss), io_service) {}

		void name_server::stop()
		{
			tcp_ns_.stop();
		}

		void name_server::remove_entry(const std::string & ability)
		{
			map_.remove(ability);
		}

		name_client::name_client(boost::asio::io_service& io_s,
						const std::string& addr, const std::string& port) :
			client(io_s)
		{
			client.connect(addr, port);
		}

		std::pair<bool, boost::asio::ip::tcp::endpoint>
		name_client::register_name(const std::string& ability)
		{
			network::register_name re;
			register_name_answer rea;

			re.name = ability;

			client.request(re, rea);

			if (rea.success) 
				return std::make_pair(true, rea.endpoint);
			return std::make_pair(false, boost::asio::ip::tcp::endpoint());
		}

		std::pair<bool, boost::asio::ip::tcp::endpoint>
		name_client::sync_resolve(const std::string& ability)
		{
			network::request_name rn;
			request_name_answer rna;

			rn.name = ability;
			client.request(rn, rna);

			if (rna.success) 
				return std::make_pair(true, rna.endpoint);
			return std::make_pair(false, boost::asio::ip::tcp::endpoint());
		}
	}
}
