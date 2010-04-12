
#ifndef _CLIENT_TCP_IMPL_HH_
#define _CLIENT_TCP_IMPL_HH_ 

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <network/socket_tcp_async_serialized.hh>

namespace hyper {
	namespace network {
		namespace tcp {

			template <typename OutputM>
			class sync_client 
			{
				public:
					explicit sync_client(boost::asio::io_service& io_service,
							const std::string& server, const std::string& port):
						resolver_(io_service),
						socket_(io_service)
					{
						/* resolv the name and connect to the requested server
						 * in synchronous way */ 
						boost::asio::ip::tcp::resolver::query query(server, port);
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
					};

					void close()
					{
						socket_.close();
					}

					/*
					 * Send a request @in and wait for an answer @out
					 * The method can throw a boost::system::system_error in failure case
					 */
					template <typename Input, typename Output>
					void sync_request(const Input& in, Output& out)
					{
						socket_.sync_write(in);
						socket_.sync_read(out);
					}

				private:
					boost::asio::ip::tcp::resolver resolver_;
					serialized_socket<OutputM> socket_;
			};
		};
	};
};



#endif /* _CLIENT_TCP_IMPL_HH_ */
