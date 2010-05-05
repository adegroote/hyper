#ifndef _NETWORK_NAMESERVER_HH_
#define _NETWORK_NAMESERVER_HH_

#include <map>
#include <string>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <network/msg.hh>
#include <network/server_tcp_impl.hh>

namespace hyper {
	namespace network {

		// Internal details
		namespace ns {
			typedef boost::mpl::vector<
									  request_name, 
									  register_name
									> input_msg;
			typedef boost::mpl::vector<
									  request_name_answer,
									  register_name_answer
									 > output_msg;

			typedef boost::make_variant_over<input_msg> input_variant;
			typedef boost::make_variant_over<output_msg> output_variant;

			// for moment, only deal with tcp
			struct addr_storage {
				boost::asio::ip::tcp::endpoint tcp_endpoint;
			};

			class map_addr : private boost::noncopyable
			{
				typedef std::map<std::string, addr_storage> addrs;
				addrs map_;
				// Locking doesn't change the semantic of map_addr
				mutable boost::shared_mutex m;

				public:
					map_addr();
					bool add(const std::string& key, const addr_storage& s);
					bool remove(const std::string& key);
					bool isIn(const std::string& key) const;
					std::pair<bool, addr_storage> get(const std::string& key) const;
			};

		};

		namespace tcp {
			struct ns_visitor : public boost::static_visitor<ns::input_variant>
			{
				ns_visitor(ns::map_addr&);
				ns::input_variant operator() (const request_name& r) const;
				ns::input_variant operator() (const register_name& r) const;

				ns::map_addr& map_;
			};
		};

		class name_server {
			typedef tcp::server<ns::input_msg, ns::output_msg, tcp::ns_visitor>
				tcp_ns_impl;

			ns::map_addr map_;
			tcp_ns_impl tcp_ns_;

			public:
				name_server(const std::string&, const std::string&);
				void run();
				void stop();
		}; 

	};
};

#endif /* _NETWORK_NAMESERVER_HH_ */
