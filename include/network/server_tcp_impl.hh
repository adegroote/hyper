#ifndef _NETWORK_SERVER_TCP_IMPL_HH_
#define _NETWORK_SERVER_TCP_IMPL_HH_

#include <set>
#include <algorithm>

#include <boost/asio/ip/host_name.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <network/socket_tcp_serialized.hh>
#include <utils/algorithm.hh>

// XXX Not portable approach
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>


namespace hyper {
	namespace network {
		namespace tcp {

			template <typename InputM, typename OutputM, typename Answer>
			class connection_manager;

			template <typename InputM, typename OutputM, typename Answer>
			struct dispatch_outbound_message;

			template <typename InputM, typename OutputM, typename Answer>
			class connection
				  : public boost::enable_shared_from_this<
									connection<InputM, OutputM, Answer> >,
				    private boost::noncopyable
			{
				public:
					typedef typename
						boost::make_variant_over<InputM>::type
						msg_variant_input;

					typedef typename
						boost::make_variant_over<OutputM>::type
						msg_variant_output;

					/* Construct a connection with the given io_service. */
					explicit connection(boost::asio::io_service& io_service,
							connection_manager<InputM, OutputM, Answer> & manager,
							const Answer& answer) :
						socket_(io_service), connection_manager_(manager), answer_(answer)
					{}

					/* Get the socket associated with the connection. */
					boost::asio::ip::tcp::socket& socket()
					{
						return socket_.socket();
					}

					/* Start the first asynchronous operation for the connection. */
					void start()
					{
						socket_.async_read(inbound_msg_,
								boost::bind(
									&connection::handle_read,
									this->shared_from_this(),
									boost::asio::placeholders::error));
					}

					/* 
					 * Stop all asynchronous operations associated with the
					 * connection.
					 */
					void stop()
					{
						socket_.close();
					}

					/* Handle completion of a write operation. */
					void handle_write(const boost::system::error_code& e)
					{
						if (!e) {
							// if succesful, just try to read another request
							start();
						} else {
							connection_manager_.stop(this->shared_from_this());
						}
					}

					template <typename T>
					void async_write(const T& t)
					{
						socket_.async_write(t,
								boost::bind(
									&connection::handle_write,
									this->shared_from_this(),
									boost::asio::placeholders::error));
					}

				private:
					/* Handle completion of a read operation. */
					void handle_read(const boost::system::error_code& e)
					{
						if (!e) {
							outbound_msg_ = boost::apply_visitor(answer_, inbound_msg_);
							dispatch_outbound_message<InputM, OutputM, Answer> dis(*this);
							boost::apply_visitor(dis, outbound_msg_);
						} else {
							connection_manager_.stop(this->shared_from_this());
						}
					}

					/* Socket for the connection */
					serialized_socket<InputM> socket_;

					/* The manager for this connection */
					connection_manager<InputM, OutputM, Answer> & connection_manager_;

					/* Incoming data are stored in a message_variant */
					msg_variant_input inbound_msg_;

					/* Outgoint data are stored in a msg variant output */
					msg_variant_output outbound_msg_;

					/* Answer visitor */
					Answer answer_;
			};

			template <typename InputM, typename OutputM, typename Answer>
			struct dispatch_outbound_message : public boost::static_visitor<void>
			{
				dispatch_outbound_message(connection<InputM, OutputM, Answer>& conn):
					conn_(conn) {};

				template <typename T>
				void operator() (const T& t) const
				{
					conn_.async_write(t);
				}

				void operator() (const boost::mpl::void_ &) const
				{
					conn_.start();
				}

				connection<InputM, OutputM, Answer>& conn_;
			};

			template <typename InputM, typename OutputM, typename Answer>
			class connection_manager : private boost::noncopyable
			{
				public:
					/* The managed connections */
					typedef typename 
						boost::shared_ptr<
										connection<InputM,
												   OutputM,
												   Answer
												   > 
										> connection_ptr;
					/* Add the specified connection to the manager and start it
					 * */
					void start(connection_ptr p)
					{
						connections_.insert(p);
						p->start();
					}

					/* Stop the specified connection */
					void stop(connection_ptr p)
					{
						connections_.erase(p);
						p->stop();
					}

					/* Stop all connections. */
					void stop_all()
					{
						std::for_each(connections_.begin(), connections_.end(),
								boost::bind(
									&connection<InputM, OutputM, Answer>::stop, _1));
						connections_.clear();
					}

				private:
					std::set<connection_ptr> connections_;
			};

			/*
			 * Provide a basic asynchronous server
			 * @InputM is the mpl::list of accepted messages in input
			 * @OutputM is the mpl::list of potential returned mesages
			 * @Answer is of kind variant visitor, which converts an input msg 
			 * of type make_variant_over<InputM> to an output msg of type
			 * make_variant_over<OutputM>.
			 */
			template<typename InputM, typename OutputM, typename Answer>
			class server : private boost::noncopyable
			{
				private:
					void init(const boost::asio::ip::tcp::endpoint& endpoint) 
					{
						acceptor_.open(endpoint.protocol());
						acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
						acceptor_.bind(endpoint);
						acceptor_.listen();
						acceptor_.async_accept(new_connection_->socket(),
						boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
					}
				public:
				  /* Construct the server to listen on specific tcp address and port */
					explicit server(const std::string& address, const std::string& port, 
						const Answer& ans, boost::asio::io_service& io_s) :
						io_service_(io_s),
						acceptor_(io_service_),
						connection_manager_(),
						new_connection_(
							new connection<InputM, OutputM, Answer> (
									io_service_, 
									connection_manager_,
									ans)),
						answer_(ans)
					{
						/* Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR). */
						boost::asio::ip::tcp::resolver resolver(io_service_);
						boost::asio::ip::tcp::resolver::query query(address, port);
						boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
						init(endpoint);
					}

					explicit server(const boost::asio::ip::tcp::endpoint &endpoint,
						const Answer& ans, boost::asio::io_service& io_s) :
						io_service_(io_s),
						acceptor_(io_service_),
						connection_manager_(),
						new_connection_(
							new connection<InputM, OutputM, Answer> (
									io_service_, 
									connection_manager_,
									ans)),
						answer_(ans)
					{
						init(endpoint);
					}

					server(short unsigned int port, 
						   const Answer& ans, boost::asio::io_service& io_s) :
						io_service_(io_s),
						acceptor_(io_service_),
						connection_manager_(),
						new_connection_(
							new connection<InputM, OutputM, Answer> (
									io_service_, 
									connection_manager_,
									ans)),
						answer_(ans)
					{
						boost::asio::ip::tcp::endpoint endpoint(
								boost::asio::ip::address_v4::any(),
								port);
						init(endpoint);
					}

					server(const Answer& ans, boost::asio::io_service& io_s) :
						io_service_(io_s),
						acceptor_(io_service_),
						connection_manager_(),
						new_connection_(
							new connection<InputM, OutputM, Answer> (
									io_service_, 
									connection_manager_,
									ans)),
						answer_(ans)
					{
						boost::asio::ip::tcp::endpoint endpoint(
								boost::asio::ip::address_v4::any(),
								0);
						init(endpoint);
					}
									
				  /* stop the server */
				  void stop()
				  {
					  /* 
					   * Post a call to the stop function so that server::stop()
					   * is safe to call from any thread. 
					   * */
					  io_service_.post(boost::bind(&server::handle_stop, this));
				  }

				  std::vector<boost::asio::ip::tcp::endpoint>
				  local_endpoints() { 
					  using namespace boost::asio;
					  ip::tcp::endpoint endpoint = acceptor_.local_endpoint();
					  ip::tcp::resolver resolver(io_service_);
					  std::ostringstream oss;
					  oss << endpoint.port();

					  struct ifaddrs * ifAddrStruct;
					  struct ifaddrs * ifa;
					  void * tmpAddrPtr;

					  std::vector<ip::tcp::endpoint> res;

					  int err = getifaddrs(&ifAddrStruct);
					  if (err < 0)
						  return res;

					  /* XXX non-portable + non-exception safe approach */
					  for (ifa = ifAddrStruct; ifa != 0; ifa = ifa->ifa_next) {
						  if (ifa -> ifa_addr == 0)
							  continue;
						  if (ifa -> ifa_addr->sa_family==AF_INET) { // only deal with ipv4
							  // is a valid IP4 Address
							  tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
							  char addressBuffer[INET_ADDRSTRLEN];
							  inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
							  ip::tcp::endpoint endp(ip::address_v4::from_string(addressBuffer),
									endpoint.port());
							  res.push_back(endp);
						  }
					  }
					  if (ifAddrStruct!=0) freeifaddrs(ifAddrStruct);

					  return res;
				  }

				private:
				  /* Handle completion of an asynchronous accept operation. */
				  void handle_accept(const boost::system::error_code& e)
				  {
					  if (!e)
					  {
						  connection_manager_.start(new_connection_);
						  new_connection_.reset(
								  new connection<InputM, OutputM, Answer> (
									  io_service_, connection_manager_,
									  answer_));
						  acceptor_.async_accept(new_connection_->socket(),
								  boost::bind(&server::handle_accept, this, 
																	 boost::asio::placeholders::error));
					  }
				  }

				  /* Handle a request to stop the server. */
				  void handle_stop()
				  {
					  /*
					   *  The server is stopped by cancelling all outstanding
					   *  asynchronous operations. Once all operations have
					   *  finished the io_service::run() call will exit.
					   */
					  acceptor_.close();
					  connection_manager_.stop_all();
				  }

				  /* The io_service used to perform asynchronous operations. */
				  boost::asio::io_service& io_service_;

				  /* Acceptor used to listen for incoming connections. */
				  boost::asio::ip::tcp::acceptor acceptor_;
				  
				  /* The connection manager which owns all live connections. */
				  connection_manager<InputM, OutputM, Answer> connection_manager_;
				  
				  /* The next connection to be accepted. */
				  boost::shared_ptr< connection<InputM, OutputM, Answer> > new_connection_;

				  /* The answer visitor */
				  Answer answer_;
			};
		}
	}
}

#endif /* _NETWORK_SERVER_TCP_IMPL_HH_ */
