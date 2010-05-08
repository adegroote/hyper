
#ifndef _CLIENT_TCP_IMPL_HH_
#define _CLIENT_TCP_IMPL_HH_ 

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <network/socket_tcp_serialized.hh>


namespace hyper {
	namespace network {
		namespace tcp {

			template <typename OutputM>
			class client 
			{
				public:
					explicit client(boost::asio::io_service& io_service):
						resolver_(io_service),
						socket_(io_service)
					{
					}

					/*
					 * Try to connect to the addr specified by @addr and @port
					 *
					 * Throw a boost::system::system_error exception in failure case
					 */
					void connect(const std::string& addr, const std::string& port)
					{
						/* resolv the name and connect to the requested server
						 * in synchronous way */ 
						boost::asio::ip::tcp::resolver::query query(addr, port);
						boost::asio::ip::tcp::resolver::iterator endpoint_iterator;
						endpoint_iterator = resolver_.resolve(query);
						boost::asio::ip::tcp::resolver::iterator end;

						// Try each endpoint until we successfully establish a connection.
						boost::system::error_code error = boost::asio::error::host_not_found;
						while (error && endpoint_iterator != end)
						{
							socket_.socket().close();
							socket_.socket().connect(*endpoint_iterator++, error);
						}
						if (error)
							throw boost::system::system_error(error);
					}

					/*
					 * Try to connect asynchronously to the addr specified by
					 * @addr and @port. Will call handler on completion
					 *
					 * Handler must implement
					 *		void (*)(const boost::system::error_code&)
					 */
					template <typename Handler>
					void async_connect(const std::string& addr, const std::string& port, 
										Handler handler)
					{
					/* 
					 * Start an asynchronous resolve to translate the server
					 * and service names into a list of endpoints.
					 */
						boost::asio::ip::tcp::resolver::query query(addr, port);

						void (client::*f)(
								const boost::system::error_code& e,
								boost::asio::ip::tcp::resolver::iterator,
								boost::tuple<Handler>)
							= &client::template handle_resolve<Handler>;

						resolver_.async_resolve(query,
								boost::bind(f, this,
									boost::asio::placeholders::error,
									boost::asio::placeholders::iterator,
									boost::make_tuple(handler)));
					}

					void close()
					{
						socket_.close();
					}

					/*
					 * Send a request @in and wait for an answer @out The
					 * method can throw a boost::system::system_error in
					 * failure case
					 */
					template <typename Input, typename Output>
					void request(const Input& in, Output& out)
					{
						socket_.sync_write(in);
						socket_.sync_read(out);
					}

					/*
					 * Send a request asynchronously @in. On completion,
					 * @handler is called. If the call is a success, @out is
					 * set with a correct value
					 *
					 * Handler must implement
					 *		void (*)(const boost::system::error_code&)
					 */
					template <typename Input, typename Output, typename Handler>
					void async_request(const Input& in, Output& out, Handler handler)
					{
						void (client::*f)(
								const boost::system::error_code& e,
								unsigned long int,
								Output&,
								boost::tuple<Handler>)
							= &client::template handle_write<Output, Handler>;

						socket_.async_write(in, 
								boost::bind(f, this,
									boost::asio::placeholders::error,
									boost::asio::placeholders::bytes_transferred,
									boost::ref(out),
									boost::make_tuple(handler)));
					}

				private:
					template <typename Handler>
					void handle_resolve(const boost::system::error_code& err,
							boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
							boost::tuple<Handler> handler)
					{
						if (!err)
						{
							// Attempt a connection to the first endpoint in the list. Each endpoint
							// will be tried until we successfully establish a connection.
							boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;

							void (client::*f)(
									const boost::system::error_code& e,
									boost::asio::ip::tcp::resolver::iterator,
									boost::tuple<Handler>)
								= &client::template handle_connect<Handler>;

							socket_.socket().async_connect(endpoint,
									boost::bind(f, this,
										boost::asio::placeholders::error,
									   	++endpoint_iterator,
										handler));
						}
						else
						{
							boost::get<0>(handler) (err);
						}
					}

					template <typename Handler>
					void handle_connect(const boost::system::error_code& err,
							boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
							boost::tuple<Handler> handler)
					{
						if (!err)
						{
							boost::get<0>(handler)(err);
						}
						else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
						{
							// The connection failed. Try the next endpoint in the list.
							socket_.close();
							boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;

							void (client::*f)(
									const boost::system::error_code& e,
									boost::asio::ip::tcp::resolver::iterator,
									boost::tuple<Handler>)
								= &client::template handle_connect<Handler>;

							socket_.socket().async_connect(endpoint,
									boost::bind(f, this,
										boost::asio::placeholders::error,
									   	++endpoint_iterator,
										handler));
						}
						else
						{
							boost::get<0>(handler)(err);
						}
					}

					template <typename Output, typename Handler>
					void handle_write(const boost::system::error_code& e, 
									  unsigned long int written,
									  Output& out,
									  boost::tuple<Handler> handler)
									  
					{
						(void) written;
						if (e)
						{
							boost::get<0>(handler) (e);
						} else {
							socket_.async_read(out, boost::get<0>(handler));
						}
					}

					boost::asio::ip::tcp::resolver resolver_;
					serialized_socket<OutputM> socket_;
			};
		};
	};
};



#endif /* _CLIENT_TCP_IMPL_HH_ */
