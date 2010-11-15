#ifndef HYPER_NETWORK_LOGGER_HH_
#define HYPER_NETWORK_LOGGER_HH_

#include <iosfwd>                          // streamsize
#include <sstream>
#include <string>

#include <network/msg.hh>
#include <network/nameserver.hh>
#include <network/client_tcp_impl.hh>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/categories.hpp>  // sink_tag
#include <boost/make_shared.hpp>


namespace io = boost::iostreams;

namespace hyper {
	namespace network {
		template <typename Resolver>
		class async_logger
		{
			private:
				boost::asio::io_service& io_s_;
				std::string src_;
				std::string dst_;
				Resolver& r_;
				name_resolve solver;
				bool connected;
				tcp::client<boost::mpl::vector<log_msg> > c;

				typedef boost::shared_ptr<log_msg> ptr_log_msg;


				void handle_write(ptr_log_msg msg_,
								  const boost::system::error_code& e)
				{
					if (e) {
						// XXX be more smart about real kind of error
						c.close();
						connected = false;
					}
				}

				void write_log(ptr_log_msg msg_)
				{
					c.async_write(*msg_, 
							boost::bind(&async_logger::handle_write, this, msg_,
										boost::asio::placeholders::error));
				}

				void handle_connect(ptr_log_msg msg_,
									const boost::system::error_code& e)
				{
					if (e) {
						// XXX what to do : log to another logger :D	
					} else {
						connected=true;
						write_log(msg_);
					}
				}

				void handle_resolve(ptr_log_msg msg_,
									const boost::system::error_code& e)
				{
					if (e) {
						// XXX what to do : log to another logger :D	
					} else {
						c.async_connect(solver.endpoint(), 
							boost::bind(&async_logger::handle_connect, this, msg_,
										boost::asio::placeholders::error));
					}
				}

			public:
				async_logger(boost::asio::io_service & io_s,  const std::string& srcAbility, 
							 const std::string& loggerAbility, Resolver& r) :
					io_s_(io_s), src_(srcAbility), dst_(loggerAbility), r_(r),
					connected(false), c(io_s)
				{}

				void log(const std::string& s)
				{
					ptr_log_msg msg_ = boost::make_shared<log_msg>(src_, s); 

					if (!connected) {

						solver.name(dst_);
					
						r_.async_resolve(solver,
								boost::bind(&async_logger::handle_resolve, this,
											 msg_,
											 boost::asio::placeholders::error));
					} else {
						write_log(msg_);
					}
				}
		};

		namespace details {
			/* Redirect thing to /dev/null */ 
			struct void_handler {
					typedef char	char_type;
					typedef io::sink_tag category;

					std::streamsize write(const char* s, std::streamsize n)
					{
						return n;
					}
			};

			template <typename Resolver>
			class remote_ability_sink {
				private:
					async_logger<Resolver> & remote_logger;

				public:
					typedef char      char_type;
					typedef io::sink_tag  category;

					remote_ability_sink(async_logger<Resolver>& remote) :
						remote_logger(remote) 
					{}

					std::streamsize write(const char* s, std::streamsize n)
					{
						remote_logger.log(s);
						return n;
					}
			};
		}

		/* 
		 * Log on remote ability dst_ every message where lvl is >= than level_
		 * The logger is usable as a classic stream
		 *
		 * log(lvl) << XXX << YYY << ...
		 */
		template <typename Resolver>
		class logger {
			public:
				logger(boost::asio::io_service& io_s, 
						const std::string& src_,
						const std::string& dst_,
						Resolver& r,
						int level) : level_(level),
									 real_(io_s, src_, dst_, r)
				{
					void_.open(details::void_handler());
					out_.open(details::remote_ability_sink<Resolver> (real_));
				}

				std::ostream& operator() (int lvl) {
					if (lvl < level_)
						return void_;
					return out_;
				}

			private:
				int level_;
				async_logger<Resolver> real_;
				io::stream<details::void_handler> void_;
				io::stream<details::remote_ability_sink<Resolver> > out_;
		};
	}
}

#endif /* HYPER_NETWORK_LOGGER_HH_ */
