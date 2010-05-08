
#ifndef _NETWORK_SOCKET_TCP_ASYNC_SERIALIZERD_HH_
#define _NETWORK_SOCKET_TCP_ASYNC_SERIALIZERD_HH_

#include <string>
#include <vector>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/variant.hpp>

#include <network/msg.hh>

namespace hyper {
	namespace network {
		namespace tcp {

			template <typename AuthorizedMessages>
			struct serialized_socket;

			/* Meta-struct to call async_read with the right @T. 
			 * @index here is the index in @AuthorizedMessages
			 * @valid tells if it must really be extented or not (valid type ..)
			 *
			 * Implementation is after the definition of serialized_socket,
			 * as we need to know its internals.
			 */
			template <typename AuthorizedMessages, typename T, typename Handler, int index, bool valid>
			struct select_read_data;

			/*
			 * Meta-struct to dispatch to real type T on the base of @index and @message_types
			 */
			template <typename AuthorizedMessages, typename Handler, int index>
			struct select_read_data_dispatch 
			{
				typedef typename boost::make_variant_over<AuthorizedMessages>::type msg_variant;

				select_read_data_dispatch(serialized_socket<AuthorizedMessages> * socket) :
					socket_(socket) {};

				void operator () (msg_variant& m, boost::tuple<Handler> handler) 
				{
					using namespace boost::mpl;
					typedef typename at_c<message_types, index>::type iter_type;
					typedef typename find<AuthorizedMessages, iter_type>::type index_authorized;
					typedef typename end<AuthorizedMessages>::type index_end;
					typedef typename boost::is_same<index_authorized, index_end>::type failure;
					typedef typename not_<failure>::type success;

					select_read_data<
						AuthorizedMessages,
						iter_type, 
						Handler,
						index_authorized::pos::value, 
						success::value
					> s(socket_);

					s(m, handler);
				};

				serialized_socket<AuthorizedMessages>* socket_;
			};

			template <typename AuthorizedMessages> // expect mpl::vector of authorized msg
			class serialized_socket{
				public:
					typedef typename boost::make_variant_over<AuthorizedMessages>::type
								msg_variant;

					serialized_socket(boost::asio::io_service& io_service) :
						socket_(io_service)
					{};

					boost::asio::ip::tcp::socket& socket() {
						return socket_;
					}

					void close() {
						return socket_.close();
					}

				private:
					template <typename T>
					void prepare_write(const T& t,
							std::vector<boost::asio::const_buffer>& buffers)
					{
						typedef typename boost::mpl::find<message_types, T>::type iter;

						header head;
						/* Get the msg type from the mpl:vector message_types */
						head.type = iter::pos::value;

						/* Compute the size archive */
						std::ostringstream archive_stream;
						boost::archive::text_oarchive archive(archive_stream);
						archive << t;
						outbound_data_ = archive_stream.str();
						head.size = (uint32_t) outbound_data_.size();

						memcpy(outbound_header_, static_cast<void*>(&head), header_length);

						/*
						 *  Write the serialized data to the socket. We use "gather-write" to send
						 *  both the header and the data in a single write operation.
						 */
						 buffers.push_back(boost::asio::buffer(outbound_header_));
						 buffers.push_back(boost::asio::buffer(outbound_data_));
					}
					

				public:
					/*
					 * This function take a message @t of type T, encode it, and then sent it
					 * On completion, @handler is called
					 *
					 * Handler must be a compatible with operation
					 *			void (*)(const boost::system::error_code&, unsigned long int)
					 */
					template <typename T, typename Handler>
					void async_write(const T& t, Handler handler)
					{
						 std::vector<boost::asio::const_buffer> buffers;
						 prepare_write(t, buffers);
						 boost::asio::async_write(socket_, buffers, handler);
					}


					/*
					 * This function takes a message @t of type T, encode it,
					 * and then sent it.
					 */
					template <typename T>
					void sync_write(const T& t)
					{
						 std::vector<boost::asio::const_buffer> buffers;
						 prepare_write(t, buffers);
						 boost::asio::write(socket_, buffers);
					}

					/*
					 * Async_read lets us read the next message, and decode it
					 * so it is usable for the application
					 *
					 * There are two variant of it :
					 *   - the first one assume an expected message type @T
					 *   - the second one assume any kind of message described
					 *   by @AuthorizedMessages so it fills a variant based on
					 *   the msg list
					 *
					 * If the message received isn't one of expected message,
					 * the method returns an invalid_argument error code
					 *
					 * On completion, @handler is called. Handler must be compatible with operation
					 *		void (*) (const boost::system::error_code&)
					 */
					template <typename T, typename Handler>
					void async_read(T& t, Handler handler)
					{
						// Issue a read operation to read exactly the number of bytes in a header.
						void (serialized_socket::*f)(
								const boost::system::error_code&,
								T&, boost::tuple<Handler>)
							= &serialized_socket::template handle_read_header<T, Handler>;

						boost::asio::async_read(socket_, boost::asio::buffer(inbound_header_),
								boost::bind(f,
									this, boost::asio::placeholders::error, boost::ref(t),
									boost::make_tuple(handler)));
					}

					template <typename Handler>
					void async_read(msg_variant& m, Handler handler)
					{
						// Issue a read operation to read exactly the number of bytes in a header.
						void (serialized_socket::*f)(
								const boost::system::error_code&,
								msg_variant&, boost::tuple<Handler>)
							= &serialized_socket::template handle_read_header<Handler>;

						boost::asio::async_read(socket_, boost::asio::buffer(inbound_header_),
								boost::bind(f,
									this, boost::asio::placeholders::error, boost::ref(m),
									boost::make_tuple(handler)));
					}

					/*
					 * Synchronously read a @T data
					 * Will throw a boost::system::error in failure case
					 */
					template <typename T>
					void sync_read(T& t)
					{
						boost::asio::read(socket_, boost::asio::buffer(inbound_header_));
						header head;
						memcpy(&head, inbound_header_, sizeof(head));

						inbound_data_.resize(head.size);

						typedef typename boost::mpl::find<message_types, T>::type right_index;

						if (head.type != right_index::pos::value)
						{
							std::cerr << "sync_read : Not acceptable msg : " << head.type << std::endl;
							boost::system::error_code error(boost::asio::error::invalid_argument);
							throw boost::system::system_error(error);
						}

						boost::asio::read(socket_, boost::asio::buffer(inbound_data_));

						try
						{
							std::string archive_data(&inbound_data_[0], inbound_data_.size());
							std::istringstream archive_stream(archive_data);
							boost::archive::text_iarchive archive(archive_stream);
							archive >> t;
						}
						catch (std::exception& e)
						{
							/* Unable to decode data. */
							std::cerr << "Unable to decode data : " << std::endl;
							boost::system::error_code error(boost::asio::error::invalid_argument);
							throw boost::system::system_error(error);
						}
					}

				private:
					/*
					 *  Handle a completed read of a message header. The handler is passed using
					 *  a tuple since boost::bind seems to have trouble binding a function object
					 *  created using boost::bind as a parameter.
					 */
					template <typename Handler>
					void handle_read_header(const boost::system::error_code& e,
								msg_variant& m, boost::tuple<Handler> handler)
					{
						if (e)
						{
							boost::get<0>(handler)(e);
						}
						else
						{
							// Determine the length of the serialized data.
							header head;
							memcpy(&head, inbound_header_, sizeof(head));

							inbound_data_.resize(head.size);
							/* 
							 * Will call the right handle_read_data, or call
							 * handler with an invalid_argument error code
							 */

							/* XXX TODO
							 * This code is pretty bad, there is probably a way to compute from
							 *  the definition of @AuthorizedMessages and @message_types a way to
							 * generate a run-time structure instead of generating this infamous
							 * switch-case with PreProcessor. 
							 *
							 * If we find a way to fix it, we can remove the depends on
							 *  MESSAGE_TYPE_MAX, and on @message_types (it will become a template
							 * parameter).
							 */
					#define DECL(z, n, text) \
						case n : {			 \
							select_read_data_dispatch<AuthorizedMessages, Handler, n> s ## n(this);	\
							return s ## n (m, handler);		\
						}

						switch (head.type) {
							BOOST_PP_REPEAT(MESSAGE_TYPE_MAX, DECL, )
							default:
								;
						}
						#undef DECL
						}
					}

					template <typename T, typename Handler>
					void handle_read_header(const boost::system::error_code& e,
							T& t, boost::tuple<Handler> handler)
					{
						if (e)
						{
							boost::get<0>(handler) (e);
						}
						else 
						{
							// Determine the length of the serialized data.
							header head;
							memcpy(&head, inbound_header_, sizeof(head));

							typedef typename boost::mpl::find<message_types, T>::type right_index;

							if (head.type != right_index::pos::value)
							{
								std::cerr << "Not acceptable msg : " << head.type << std::endl;
								boost::system::error_code error(boost::asio::error::invalid_argument);
								boost::get<0>(handler)(error);
								return;
							}

							// Issue a read operation to read exactly the number of bytes of the struct
							inbound_data_.resize(head.size);
							void (serialized_socket::*f)(
									const boost::system::error_code&,
									T&, boost::tuple<Handler>)
								= &serialized_socket::template handle_read_data<T, Handler>;

							boost::asio::async_read(socket_, boost::asio::buffer(inbound_data_),
									boost::bind(f,
										this, boost::asio::placeholders::error, boost::ref(t),
										handler));
						}
					}

					/* Handle a completed read of message data. */
					template <typename T, typename Handler>
					int handle_read_data_(const boost::system::error_code& e,
							T& t, boost::tuple<Handler> handler)
					{
						if (e)
						{
							boost::get<0>(handler)(e);
							return -1;
						}
						else
						{
							/* Extract the data structure from the data just received. */
							try
							{
								std::string archive_data(&inbound_data_[0], inbound_data_.size());
								std::istringstream archive_stream(archive_data);
								boost::archive::text_iarchive archive(archive_stream);
								archive >> t;
							}
							catch (std::exception& e)
							{
								/* Unable to decode data. */
								boost::system::error_code error(boost::asio::error::invalid_argument);
								boost::get<0>(handler)(error);
								return -1;
							}

							return 0;
						}
					}

					template <typename T, typename Handler>
					void handle_read_data(const boost::system::error_code& e,
							T& t, boost::tuple<Handler> handler)
					{
						int res = handle_read_data_(e, t, handler);
						/* Inform caller that data has been received ok. */
						if (res == 0)
							boost::get<0>(handler)(e);
					}

					template <typename T, typename Handler>
					void handle_read_data(const boost::system::error_code& e,
								T& t, msg_variant& m, boost::tuple<Handler> handler)
					{
						int res = handle_read_data_(e, t, handler);
						if (res == 0) {
							m = t;
							/* Inform caller that data has been received ok. */
							boost::get<0>(handler)(e);
						}
					}

					template <typename Authorized, typename T, typename Handler,
							   int index, bool valid> friend struct select_read_data;

					/* The different kind of messages acceptables */
					typedef typename boost::fusion::result_of::as_vector<AuthorizedMessages>::type
							type_msgs;
					type_msgs msgs;

					/* The underlying socket. */
					boost::asio::ip::tcp::socket socket_;
					
					/* The size of a fixed length header.*/
					enum { header_length = sizeof(header) };
					
					/* Holds an outbound header. */
					char outbound_header_[header_length];
					
					/* Holds the outbound data. */
					std::string outbound_data_;
					
					/* Holds an inbound header. */
					char inbound_header_[header_length];

					/* Holds the inbound data. */
					std::vector<char> inbound_data_;
			};


			template <typename AuthorizedMessages, typename T, typename Handler, int index>
			struct select_read_data<AuthorizedMessages, T, Handler, index, false> 
			{
				typedef typename boost::make_variant_over<AuthorizedMessages>::type msg_variant;

				select_read_data(serialized_socket<AuthorizedMessages> * socket) 
				{
					(void) socket;
				};

				void operator() (msg_variant& m, boost::tuple<Handler> handler) 
				{
					(void) m;
					std::cerr << "Not acceptable msg : " << index << std::endl;
					boost::system::error_code error(boost::asio::error::invalid_argument);
					boost::get<0>(handler)(error);
				};
			};

			template <typename AuthorizedMessages, typename T, typename Handler, int index>
			struct select_read_data<AuthorizedMessages, T, Handler, index, true> 
			{
				typedef serialized_socket<AuthorizedMessages> socket_type;
				typedef typename socket_type::msg_variant msg_variant;

				select_read_data(socket_type * socket) :
					socket_(socket) {};

				void operator() (msg_variant&m, boost::tuple<Handler> handler) 
				{
					void (socket_type::*f)(
							const boost::system::error_code&,
							T&,
							msg_variant&,
							boost::tuple<Handler>);
					f = & (socket_type::template handle_read_data<T, Handler>);
					boost::asio::async_read(socket_->socket_, 
							boost::asio::buffer(socket_->inbound_data_),
							boost::bind(f,
								socket_, boost::asio::placeholders::error,
								boost::ref( boost::fusion::at_c<index> (socket_->msgs)),
								boost::ref(m),
								handler));
				}

				socket_type* socket_;
			};
		};
	};
};

#endif /* _NETWORK_SOCKET_TCP_ASYNC_SERIALIZERD_HH_ */
