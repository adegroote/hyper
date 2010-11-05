#include <network/ping.hh>

#include <boost/system/system_error.hpp>

using namespace hyper::network;

ping_process::ping_process(boost::asio::io_service& io_s,
						   boost::posix_time::time_duration delay,
						   const std::string& abilityName,
						   const std::string& destName,
						   const std::string destPort) :
	is_connected(false),
	destName_(destName),
	destPort_(destPort),
	io_service_(io_s),
	delay_(delay),
	c_(io_service_),
	deadline_(io_service_) 
{
	msg_.value = 0;
	msg_.name = abilityName;
}

void ping_process::handle_write(const boost::system::error_code& e)
{
	if (e) {
		throw boost::system::system_error(e);
	} else {
		/* Rearm the timer, and let's go for another loop */
		run();
	}
}

void ping_process::handle_connect(const boost::system::error_code& e)
{
	if (e) {
		throw boost::system::system_error(e);
	} else {
		is_connected = true;
		msg_.value++;
		c_.async_write(msg_ , boost::bind(&ping_process::handle_write,
									  this,
									  boost::asio::placeholders::error));
	}
}

void ping_process::handle_timeout(const boost::system::error_code& e)
{
	if (e) {
		throw boost::system::system_error(e);
	} else {
		if (!is_connected) {
			c_.async_connect(destName_, destPort_, 
							 boost::bind(&ping_process::handle_connect,
										 this,
										 boost::asio::placeholders::error));
		} else {
			handle_connect(e);
		}
	}
}

void ping_process::run()
{
	deadline_.expires_from_now(delay_);
	deadline_.async_wait(boost::bind(&ping_process::handle_timeout, 
									  this,
									  boost::asio::placeholders::error));
}

void ping_process::stop()
{
	deadline_.cancel();
}


