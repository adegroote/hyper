#include <algorithm>

#include <boost/bind.hpp>

#include <network/nameserver_tcp_impl.hh>

using namespace hyper::network::tcp_async;

void connection_manager::start(connection_ptr p)
{
	connections_.insert(p);
	p->start();
}

void connection_manager::stop(connection_ptr p)
{
	connections_.erase(p);
	p->stop();
}

void connection_manager::stop_all()
{
	std::for_each(connections_.begin(), connections_.end(), boost::bind(&connection::stop, _1));
	connections_.clear();
}

connection::connection(boost::asio::io_service& io_service, connection_manager& manager) :
	socket_(io_service), connection_manager_(manager) 
{}

boost::asio::ip::tcp::socket& connection::socket()
{
	  return socket_;
}

void connection::start()
{
	socket_.async_read_some(boost::asio::buffer(buffer_),
			boost::bind(&connection::handle_read, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}

void connection::stop()
{
	  socket_.close();
}

void connection::handle_read(const boost::system::error_code& e, std::size_t bytes_transferred)
{
	if (!e) {
		boost::asio::async_write(socket_, boost::asio::buffer(buffer_),
				boost::bind(&connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error));
	} else {
		connection_manager_.stop(shared_from_this());
	}
}

void connection::handle_write(const boost::system::error_code& e)
{
	if (!e) {
		// if succesful, just try to read another request
		start();
	} else {
		connection_manager_.stop(shared_from_this());
	}
}


server::server(const std::string& address, const std::string& port) :
	io_service_(),
	acceptor_(io_service_),
	connection_manager_(),
	new_connection_(new connection(io_service_, connection_manager_))
{
	/* Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR). */
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(address, port);
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();
	acceptor_.async_accept(new_connection_->socket(),
			boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
}

void server::run()
{
	io_service_.run();
}

void server::stop()
{
	/* Post a call to the stop function so that server::stop() is safe to call from any thread. */
	io_service_.post(boost::bind(&server::handle_stop, this));
}
	  
void server::handle_accept(const boost::system::error_code& e)
{
	if (!e)
	{
		connection_manager_.start(new_connection_);
		new_connection_.reset(new connection(io_service_, connection_manager_));
		acceptor_.async_accept(new_connection_->socket(),
				boost::bind(&server::handle_accept, this, boost::asio::placeholders::error));
	}
}

void server::handle_stop()
{
	/*
	 *  The server is stopped by cancelling all outstanding asynchronous
	 *  operations. Once all operations have finished the io_service::run() call
	 *  will exit.
	 */
	acceptor_.close();
	connection_manager_.stop_all();
}


