#include <network/nameserver.hh>

using namespace hyper;

namespace details {
	typedef boost::mpl::vector<network::request_name,
							   network::register_name,
							   network::ping
							   > input_msg;
	typedef boost::mpl::vector<network::request_name_answer,
								network::register_name_answer,
								boost::mpl::void_ 
							    > output_msg;

	typedef boost::make_variant_over<input_msg>::type input_variant;
	typedef boost::make_variant_over<output_msg>::type output_variant;

	struct runtime_visitor : public boost::static_visitor<output_variant>
	{
		network::tcp::ns_visitor& ns_vis;

		runtime_visitor(network::tcp::ns_visitor& ns_vis_) : ns_vis(ns_vis_) {}

		template <typename T>
		output_variant operator() (const T& t) const
		{
			return ns_vis(t);
		}

		output_variant operator() (const network::ping& p) const
		{
			std::cout << "receive ping message " << p.value << std::endl;
			return boost::mpl::void_();
		}
	};
}


int main()
{
	boost::asio::io_service io_s;
	network::ns::map_addr map;
	network::tcp::ns_port_generator gen("5000");
	network::tcp::ns_visitor ns_vis(map, gen, std::cout);
	details::runtime_visitor runtime_vis(ns_vis);

	typedef network::tcp::server<details::input_msg, 
								  details::output_msg, 
								  details::runtime_visitor
								> tcp_runtime_impl;
	tcp_runtime_impl runtime("localhost", "4242", runtime_vis, io_s);

	io_s.run();
}
