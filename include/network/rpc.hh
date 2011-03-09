#ifndef HYPER_NETWORK_RPC_HH_
#define HYPER_NETWORK_RPC_HH_

#include <string>

#include <boost/optional/optional.hpp>

#include <network/nameserver.hh>
#include <network/client_tcp_impl.hh>

namespace hyper {
	namespace network {
		template <typename Input, typename Output, typename Resolver>
		class async_rpc
		{
				/* 
				 * Expect Output as a unique type, we may handle Output as a
				 * mpl::sequence, and in this case, the res_type may be a
				 * boost::optional<variant<> > 
				 */
			public:
				typedef Output res_type;
				typedef boost::optional<Output> real_res_type;

			private:
				res_type value_;
				real_res_type value;
				bool finished;
				std::string ability_;
				bool connected;

				tcp::client<boost::mpl::vector<Output> > c;
				Resolver& r_;
				name_resolve solver;

				template <typename Handler>
				void send_request(const Input& input, 
								  boost::tuple<Handler> handler)
				{
					void (async_rpc::*f) (const boost::system::error_code&,
							boost::tuple<Handler>) =
						&async_rpc::template handle_request<Handler>;

					c.async_request(input, value_, 
							boost::bind(f, this, boost::asio::placeholders::error, handler));
				}

				template <typename Handler>
				void handle_request(const boost::system::error_code &e,
									boost::tuple<Handler> handler)
				{
					finished = true;
					if (!e) 
						value = value_;
					boost::get<0>(handler)(e);
				}

				template <typename Handler>
				void handle_connect(const Input& input,
									const boost::system::error_code& e,
									boost::tuple<Handler> handler)
				{
					if (e) 
						boost::get<0>(handler)(e);
					else {
						connected=true;
						send_request<Handler> (input, handler);
					}
				}

				template <typename Handler>
				void handle_resolve(const Input& input,
									const boost::system::error_code& e,
									boost::tuple<Handler> handler)
				{
					if (e) 
						boost::get<0>(handler)(e);
					else {
						void (async_rpc::*f) (
								const Input& input,
								const boost::system::error_code&,
								boost::tuple<Handler>) =
							&async_rpc::template handle_connect<Handler>;

						c.async_connect(solver.endpoints(), 
							boost::bind(f, this, boost::cref(input), 
										boost::asio::placeholders::error, handler));
					}
				}

			public:
				async_rpc(boost::asio::io_service & io_s,  const std::string& ability, 
						 Resolver& r) :
					value(boost::none),
					finished(false), 
					ability_(ability),
					connected(false),
					c(io_s), r_(r) {}

				template <typename Handler>
				void compute(const Input& input,  Handler handler)
				{
					finished = false;
					value = boost::none;

					if (!connected) {
					void (async_rpc::*f) (
							const Input& input,
							const boost::system::error_code&,
							boost::tuple<Handler>) =
						&async_rpc::template handle_resolve<Handler>;
					solver.name(ability_);
					
					r_.async_resolve(solver,
							boost::bind(f, this,
										 boost::cref(input),
										 boost::asio::placeholders::error,
										 boost::make_tuple(handler)));
					} else {
						send_request<Handler> (input, boost::make_tuple(handler));
					}
				}

				void abort() { c.close(); };

				const real_res_type& operator() () const { return value; };

				bool is_finished() const { return finished; };
		};

		template <typename Input, typename Output, typename Resolver>
		class sync_rpc
		{
			private:
				boost::asio::io_service& io_service_;
				boost::asio::deadline_timer deadline_;
				async_rpc<Input, Output, Resolver> rpc_;

			public:
				sync_rpc(boost::asio::io_service& io_s, const std::string& ability,
						 Resolver &r):
					io_service_(io_s),
					deadline_(io_s),
					rpc_(io_s, ability, r)
				{

				deadline_.expires_at(boost::posix_time::pos_infin);

				// Start the persistent actor that checks for deadline expiry.
				check_deadline();
				}

				const boost::optional<Output>& 
				compute(const Input& input, boost::posix_time::time_duration timeout)
				{
					deadline_.expires_from_now(timeout);
					boost::system::error_code ec = boost::asio::error::would_block;

					rpc_.compute(input, 
								boost::bind(&sync_rpc::handle_answer, 
								this,
								boost::asio::placeholders::error,
								boost::ref(ec))
							);

					// Block until the asynchronous operation has completed.:D
					do io_service_.run_one(); while (ec == boost::asio::error::would_block);

					deadline_.expires_at(boost::posix_time::pos_infin);

					return rpc_();
				}

			private:
			  void check_deadline()
			  {
				  // Check whether the deadline has passed. We compare the deadline against
				  // the current time since a new asynchronous operation may have moved the
				  // deadline before this actor had a chance to run.
				  if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
				  {
					  rpc_.abort();
					  deadline_.expires_at(boost::posix_time::pos_infin);
				  }
					
				// Put the actor back to sleep.
				deadline_.async_wait(boost::bind(&sync_rpc::check_deadline, this));
			}

			  void handle_answer(const boost::system::error_code& e,
								 boost::system::error_code& wait_e)
			  {
				  wait_e = e;
			  }
		};
	}
}

#endif /* HYPER_NETWORK_RPC_HH_ */
