#ifndef HYPER_NETWORK_LOGGER_HH_
#define HYPER_NETWORK_LOGGER_HH_

#include <iosfwd>                          // streamsize
#include <deque>
#include <sstream>
#include <string>

#include <network/msg_log.hh>
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

				std::deque<log_msg> msgs_;

				void handle_write(const boost::system::error_code& e)
				{
					if (e) {
						// XXX be more smart about real kind of error
						c.close();
						connected = false;
					}
				}

				void write_log()
				{
					while (!msgs_.empty()) {
						c.async_write(msgs_.front(), 
								boost::bind(&async_logger::handle_write, this, 
											boost::asio::placeholders::error));
						msgs_.pop_front();
					}
				}

				void handle_connect(const boost::system::error_code& e)
				{
					if (e) {
						// XXX what to do : log to another logger :D	
					} else {
						connected=true;
						write_log();
					}
				}

				void handle_resolve(const boost::system::error_code& e)
				{
					if (e) {
						// XXX what to do : log to another logger :D	
					} else {
						c.async_connect(solver.endpoints(), 
							boost::bind(&async_logger::handle_connect, this, 
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
					msgs_.push_back(log_msg(src_, s));
					if (msgs_.size() != 1)
						return;

					if (!connected) {
						solver.name(dst_);
					
						r_.async_resolve(solver,
								boost::bind(&async_logger::handle_resolve, this,
											 boost::asio::placeholders::error));
					} else {
						write_log();
					}
				}
		};

		namespace details {
			/* Redirect thing to /dev/null */ 
			struct void_handler {
					typedef char	char_type;
					typedef io::sink_tag category;

					std::streamsize write(const char*, std::streamsize n)
					{
						return n;
					}
			};

			template <typename Resolver>
			class remote_ability_sink {
				private:
					std::string str;
					async_logger<Resolver> & remote_logger;

				public:
					typedef char      char_type;
					typedef io::sink_tag  category;

					remote_ability_sink(async_logger<Resolver>& remote) :
						remote_logger(remote) 
					{}

					std::streamsize write(const char* s, std::streamsize n)
					{
						str.clear();
						str.insert(0, s, n);
						remote_logger.log(str);
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
					if (lvl > level_)
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
