#include <network/nameserver.hh>

using namespace hyper::network;

int main()
{
	boost::asio::io_service io_s;
	name_server ns("localhost", "4242", io_s);

	io_s.run();
}
