#ifndef _NETWORK_NAMESERVER_TCP_IMPL_HH_
#define _NETWORK_NAMESERVER_TCP_IMPL_HH_

#include <set>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace hyper {
	namespace network {
		namespace tcp_async {

			class connection_manager;

			class connection
				  : public boost::enable_shared_from_this<connection>,
				    private boost::noncopyable
			{
				public:
					/* Construct a connection with the given io_service. */
					explicit connection(boost::asio::io_service& io_service, connection_manager& manager);

					/* Get the socket associated with the connection. */
					boost::asio::ip::tcp::socket& socket();

					/* Start the first asynchronous operation for the connection. */
					void start();

					/* Stop all asynchronous operations associated with the connection. */
					void stop();

				private:
					/* Handle completion of a read operation. */
					void handle_read(const boost::system::error_code& e, std::size_t bytes_transferred);

					/* Handle completion of a write operation. */
					void handle_write(const boost::system::error_code& e);

					/* Socket for the connection */
					boost::asio::ip::tcp::socket socket_;

					/* The manager for this connection */
					connection_manager& connection_manager_;

					/* Buffer for incoming data. */
					boost::array<char, 8192> buffer_;
			};

			typedef boost::shared_ptr<connection> connection_ptr;

			class connection_manager : private boost::noncopyable
			{
				public:
					/* Add the specified connection to the manager and start it */
					void start(connection_ptr c);

					/* Stop the specified connection */
					void stop(connection_ptr c);

					/* Stop all connections. */
					void stop_all();

				private:
					/* The managed connections */
					std::set<connection_ptr> connections_;
			};

			class server : private boost::noncopyable
			{
				public:
				  /* Construct the server to listen on specific tcp address and port */
				  explicit server(const std::string& address, const std::string& port);

				  /* start the io_service loop */
				  void run();

				  /* stop the server */
				  void stop();

				private:
				  /* Handle completion of an asynchronous accept operation. */
				  void handle_accept(const boost::system::error_code& e);

				  /* Handle a request to stop the server. */
				  void handle_stop();

				  /* The io_service used to perform asynchronous operations. */
				  boost::asio::io_service io_service_;

				  /* Acceptor used to listen for incoming connections. */
				  boost::asio::ip::tcp::acceptor acceptor_;
				  
				  /* The connection manager which owns all live connections. */
				  connection_manager connection_manager_;
				  
				  /* The next connection to be accepted. */
				  connection_ptr new_connection_;
			};
		};
	};
};

#endif /* _NETWORK_NAMESERVER_TCP_IMPL_HH_ */
