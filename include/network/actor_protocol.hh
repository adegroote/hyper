#ifndef HYPER_NETWORK_ACTOR_PROTOCOL_HH
#define HYPER_NETWORK_ACTOR_PROTOCOL_HH

#include <string>
#include <map>

#include <boost/function.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/void.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>

#include <network/log.hh>
#include <network/msg.hh>
#include <network/nameserver.hh>
#include <network/client_tcp_impl.hh>

namespace hyper {
	namespace network {
		/* InputM is a mpl sequence of acceptable input type */
		template <typename InputM>
		class callback_database 
		{
			public:
				typedef boost::function<void ()> fun_cb;
				typedef typename boost::make_variant_over<InputM>::type msg_variant;

			private:
				struct cb_info {
					fun_cb cb;
					msg_variant input;
					std::string actor_dst;
				};

				typedef std::set<identifier> set_id;

				std::map<identifier, cb_info> map_cb;
				std::map<std::string, set_id> id_by_actor;

				void remove_helper(identifier id)
				{
					map_cb.erase(id);
				}


			public:
				void add(const std::string& actor, identifier id, fun_cb cb)
				{
					cb_info info;
					info.cb = cb;
					info.actor_dst = actor;

					map_cb[id] = info;
					id_by_actor[actor].insert(id);
				}
				
				template <typename T>
				void trigger(identifier id, const T& input)
				{
					using namespace boost::mpl;
					typedef typename find<InputM, T>::type index;
					typedef typename end<InputM>::type index_end;
					BOOST_MPL_ASSERT_NOT(( boost::is_same< index, index_end > ));

					cb_info& info = map_cb[id];
					info.input = input;
					info.cb();
				}

				template <typename T>
				T get_input(identifier id)
				{
					cb_info& info = map_cb[id];
					return boost::get<T>(info.input);
				}

				void remove(identifier id)
				{
					typename std::map<identifier, cb_info>::iterator it;
					it = map_cb.find(id);
					assert(it != map_cb.end());

					id_by_actor[it->second.actor_dst].erase(id);
					remove_helper(id);
				}

				void cancel(const std::string& actor)
				{
					set_id& s = id_by_actor[actor];
					std::for_each(s.begin(), s.end(), 
								  boost::bind(&callback_database::remove_helper, this, _1));
				}
		};

		/* Actor must have :
		 *	- a name (std::string)
		 *	- a boost::asio::io_service io_s, running
		 *	- a method identifier gen_identifier()
		 *	- a callback_database db to deal with async request
		 *	- a resolver name_client
		 *	- a logger logger
		 */

		template <typename Input>
		std::string actor_identifier(const Input & input)
		{
			std::ostringstream oss;
			oss << "[" << input.src << ", " << input.id << "]";
			return oss.str();
		}

		/* actor_protocol_visitor only accepts message with an id field */
		template <typename Actor>
		struct actor_protocol_visitor : public boost::static_visitor<boost::mpl::void_>
		{
			Actor& actor;
			actor_protocol_visitor(Actor& actor_) : actor(actor_) {}

			template <typename T>
			boost::mpl::void_ operator() (const T& t) const
			{
				identifier id = t.id;
				actor.logger(DEBUG_PROTOCOL) << actor_identifier(t);
				actor.logger(DEBUG_PROTOCOL) << " triggered" << std::endl;
				actor.db.trigger(id, t);
				return boost::mpl::void_();
			}
		};

		template <typename Actor>
		class actor_client
		{
			private:
				Actor& actor;
				tcp::client<boost::mpl::vector<> > c_;
				bool connected;
				std::string actor_dst;
				name_resolve solver;

				template <typename Handler>
				void handle_basic_write(const boost::system::error_code& e,
										boost::tuple<Handler> handler)
				{
					if (e)
						connected = false;
					boost::get<0>(handler) (e);
				}

				template <typename Handler>
				void handle_write(const boost::system::error_code& e,
									identifier id, 
									boost::tuple<Handler> handler)
				{
					if (e)
					{
						actor.logger(DEBUG_PROTOCOL) << "[" << actor.name << ", " << id;
						actor.logger(DEBUG_PROTOCOL) << " ] Writing failed" << std::endl;
						actor.db.remove(id);
						boost::get<0>(handler) (e);
					}
				}

				template <typename Output, typename Handler>
				void handle_request(
									 identifier id,
									 Output& output,
									 boost::tuple<Handler> handler)
				{
					actor.logger(DEBUG_PROTOCOL) << "[" << actor.name << ", " << id;
					actor.logger(DEBUG_PROTOCOL) << "] Callback called " << std::endl;
					boost::system::error_code e;
					try {
						 output =  actor.db.template get_input<Output>(id);
					} catch (const boost::bad_get&)
						{
							actor.logger(DEBUG_PROTOCOL) << "[" << actor.name << ", " << id;
							actor.logger(DEBUG_PROTOCOL) << "] Callback invalid answer " << std::endl;
							e  = boost::asio::error::invalid_argument;
						}
					actor.db.remove(id);
					boost::get<0>(handler)(e);
				}

				template <typename Input, typename Handler>
				void write(const Input& input, boost::tuple<Handler> handler)
				{
					void (actor_client::*f) (const boost::system::error_code&,
							boost::tuple<Handler>) =
						&actor_client::template handle_basic_write<Handler>;

					actor.logger(DEBUG_PROTOCOL) << actor_identifier(input) << " Writing " << std::endl;
					c_.async_write(input,
							boost::bind(f, this,
										   boost::asio::placeholders::error, handler));
				}

				template <typename Input, typename Handler>
				void handle_connect(const boost::system::error_code& e,
									const Input& input,
									boost::tuple<Handler> handler)
				{
					if (e) 
						boost::get<0>(handler)(e);
					else {
						connected=true;
						write (input, handler);
					}
				}

				template <typename Input, typename Handler>
				void handle_resolve(const boost::system::error_code& e,
									const Input& input,
									boost::tuple<Handler> handler)
				{
					if (e) 
						boost::get<0>(handler)(e);
					else {
						void (actor_client::*f) (const boost::system::error_code&,
								const Input& input,
								boost::tuple<Handler>) =
							&actor_client::template handle_connect<Input, Handler>;

						c_.async_connect(solver.endpoint(), 
							boost::bind(f, this, 
										boost::asio::placeholders::error, 
										boost::cref(input),
										handler));
					}
				}

				template <typename Input, typename Handler>
				void async_write_(const Input& input, boost::tuple<Handler> handler)
				{
					if (!connected) {
					void (actor_client::*f) (const boost::system::error_code&,
							const Input& input,
							boost::tuple<Handler>) =
						&actor_client::template handle_resolve<Input, Handler>;
					solver.name(actor_dst);
					
					actor.name_client.async_resolve(solver,
							boost::bind(f, this, 
										 boost::asio::placeholders::error,
										 boost::cref(input),
										 handler));
					} else {
						write(input, handler);
					}
				}


			public:
				actor_client(Actor& actor_, const std::string& dst) :
					actor(actor_), c_(actor.io_s), connected(false), actor_dst(dst)
				{}

				template <typename Input, typename Handler>
				void async_write(const Input& input, Handler handler)
				{
					return async_write_(input, boost::make_tuple(handler));
				}

				template <typename Input, typename Output, typename Handler>
				void async_request(const Input& input, 
								   Output& output, Handler handler)
				{
					input.id = actor.gen_identifier();
					input.src = actor.name;

					void (actor_client::*write_cb)(
							const boost::system::error_code& e,
							identifier id,
							boost::tuple<Handler>)
						= &actor_client::template handle_write<Handler>;

					void (actor_client::*request_cb)(
							identifier id,
							Output &,
							boost::tuple<Handler>)
						= &actor_client::template handle_request<Output, Handler>;

					actor.db.add(actor.name, input.id, 
								 boost::bind(request_cb,
											 this, input.id, boost::ref(output),
											 boost::make_tuple(handler)));

					async_write(input, boost::bind(write_cb,
												 this, 
												 boost::asio::placeholders::error,
												 input.id, boost::make_tuple(handler)));
				}
		};

		template <typename Actor>
		class actor_client_database 
		{
			private:
				typedef actor_client<Actor> type;
				typedef boost::shared_ptr<type> ptr_type;
				typedef std::map<std::string, ptr_type > db_type;
				typedef typename db_type::iterator iterator;
				typedef typename db_type::const_iterator const_iterator;

				Actor &actor;
				db_type db;

			public:
				actor_client_database(Actor & actor_) : actor(actor_) {}
				actor_client<Actor>& operator[](const std::string& s)
				{
					iterator it = db.find(s);
					if (it != db.end())
						return *(it->second);

					std::pair<iterator, bool> p;
					p = db.insert(std::make_pair(s, ptr_type(new type(actor, s))));
					assert (p.second);
					return *(p.first->second);
				}
		};
	}
}

#endif /* HYPER_NETWORK_ACTOR_PROTOCOL_HH */
