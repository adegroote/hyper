#ifndef HYPER_NETWORK_PING_HH_
#define HYPER_NETWORK_PING_HH_

#include <network/client_tcp_impl.hh>

namespace hyper {
	namespace network {
		class ping_process {

			/* 
			 * ping_process can throw a boost::system::system_error exception
			 * if something fails (in particular, if the remote host dies).
			 */
			public:
				explicit ping_process(boost::asio::io_service& io_s,
							 boost::posix_time::time_duration delay,
							 const std::string& abilityName,
							 const std::string& destName,
							 const std::string destPort);

				void run();

				void stop();

			private:

			  void handle_timeout(const boost::system::error_code&);
			  void handle_connect(const boost::system::error_code&);
			  void handle_write(const boost::system::error_code&);

			  bool is_connected;
			  std::string destName_;
			  std::string destPort_;

			  boost::asio::io_service& io_service_;
			  boost::posix_time::time_duration delay_;
			  network::tcp::client<boost::mpl::vector<> > c_;
			  boost::asio::deadline_timer deadline_;
			  ping msg_;
		};
	}
}

#endif /* HYPER_NETWORK_PING_HH_ */
