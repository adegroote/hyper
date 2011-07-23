#include <vector>
#include <string>

#include <network/msg.hh>
#include <network/ping.hh>
#include <network/nameserver.hh>
#include <network/server_tcp_impl.hh>

#include <model/discover_root.hh>


namespace {
	typedef boost::mpl::vector<hyper::network::log_msg,
							   hyper::network::inform_new_agent,
							   hyper::network::inform_death_agent> input_msg;
	typedef boost::mpl::vector<boost::mpl::void_> output_msg;

	typedef boost::make_variant_over<input_msg>::type input_variant;
	typedef boost::make_variant_over<output_msg>::type output_variant;

	struct logger_visitor : public boost::static_visitor<output_variant>
	{
		std::vector<hyper::network::log_msg> & log_msgs_;

		logger_visitor(std::vector<hyper::network::log_msg>& log_msgs): log_msgs_(log_msgs) {}

		output_variant operator() (const hyper::network::log_msg& msg) const
		{
			log_msgs_.push_back(msg);
			return boost::mpl::void_();
		}

		output_variant operator() (const hyper::network::inform_death_agent& msg) const
		{
			// don't care but avoids some warning message
			std::ostringstream oss;
			oss << "Following agent(s) die : ";
			std::copy(msg.dead_agents.begin(), msg.dead_agents.end(),
					  std::ostream_iterator<std::string>(oss, ", "));
			hyper::network::log_msg log("root", oss.str());
			log_msgs_.push_back(log);

			return boost::mpl::void_();
		}

		output_variant operator() (const hyper::network::inform_new_agent& msg) const
		{
			std::ostringstream oss;
			oss << "New agent(s) in the system : ";
			std::copy(msg.new_agents.begin(), msg.new_agents.end(),
					  std::ostream_iterator<std::string>(oss, ", "));
			hyper::network::log_msg log("root", oss.str());
			log_msgs_.push_back(log);

			return boost::mpl::void_();
		}
	};

	struct cmp_log_msg_by_date
	{
		bool operator() (const hyper::network::log_msg& msg1, 
						 const hyper::network::log_msg& msg2) const
		{
			return (msg1.date < msg2.date);
		}
	};

	struct too_young_msg
	{
		boost::posix_time::time_duration delay_;

		too_young_msg(boost::posix_time::time_duration delay) : delay_(delay) {}

		bool operator() (const hyper::network::log_msg msg) const
		{
			boost::posix_time::ptime oldest(boost::posix_time::microsec_clock::local_time());
			oldest -= delay_;
			return (msg.date > oldest);
		}
	};

	struct display_log 
	{
		void operator() (const hyper::network::log_msg& msg)
		{
			std::cout << "[" << boost::posix_time::to_iso_string(msg.date) << "]";
			std::cout << "[" << msg.src << "] ";
			std::cout << msg.msg << std::endl;
		}
	};

	struct periodic_check
	{
		boost::asio::io_service& io_s_;

		boost::posix_time::time_duration delay_;
		boost::asio::deadline_timer timer_;

		std::vector<hyper::network::log_msg> & log_msgs_;


		periodic_check(boost::asio::io_service& io_s, 
					   boost::posix_time::time_duration delay,
					   std::vector<hyper::network::log_msg>& log_msgs):
			io_s_(io_s), delay_(delay), timer_(io_s_),
			log_msgs_(log_msgs)
		{}

		void handle_timeout(const boost::system::error_code& e)
		{
			/* Make a chance to reorder msg in the right way */
			if (!e) {
				std::sort(log_msgs_.begin(), log_msgs_.end(), cmp_log_msg_by_date());
				std::vector<hyper::network::log_msg>::iterator it;
				it = std::find_if(log_msgs_.begin(), log_msgs_.end(), too_young_msg(delay_*2));
				if (it != log_msgs_.begin())
				{
					std::for_each(log_msgs_.begin(), it, display_log());
					log_msgs_.erase(log_msgs_.begin(), it); 
				}

				run();
			}
		}

		void run()
		{
			timer_.expires_from_now(delay_);
			timer_.async_wait(boost::bind(&periodic_check::handle_timeout, 
									  this,
									  boost::asio::placeholders::error));
		}
	};
}

int main()
{
	typedef hyper::network::tcp::server<input_msg, output_msg, logger_visitor> logger_server;
	boost::asio::io_service io_s;
	std::vector<hyper::network::log_msg> msgs;

	hyper::model::discover_root discover;
	hyper::network::name_client name_client_(io_s, discover.root_addr(), discover.root_port());
	logger_visitor vis(msgs);
	logger_server serv(vis, io_s);
	bool res = name_client_.register_name("logger", serv.local_endpoints());
	if (res == false) {
		std::cerr << "Failed to register logger, exiting ... " << std::endl;
		return -1;
	}

	periodic_check check(io_s, boost::posix_time::milliseconds(100), msgs);
	hyper::network::ping_process ping(io_s, boost::posix_time::milliseconds(100),
									  "logger", discover.root_addr(), discover.root_port());

	ping.run();
	check.run();
	io_s.run();
}
