#ifndef HYPER_NETWORK_ALGORITHM_HH_
#define HYPER_NETWORK_ALGORITHM_HH_

#include <vector>

#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/system/error_code.hpp>
#include <boost/tuple/tuple.hpp>

#include <utils/algorithm.hh>

namespace hyper {
	namespace network {
		namespace details {
			template <typename It, typename Func, typename Handler>
			void handle_async_for_each(const boost::system::error_code& e, 
									   It begin, It end, Func func, boost::tuple<Handler> handler)
			{
				if (e) 
					return boost::get<0>(handler)(e);

				++begin;
				if (begin == end) 
					return boost::get<0>(handler)(boost::system::error_code());

				void (*f)(const boost::system::error_code& e,
						  It begin, It end, Func func, boost::tuple<Handler>) =
					&handle_async_for_each<It, Func, Handler>;

				func(*begin, boost::bind(f, boost::asio::placeholders::error,
											begin, end, func, handler));

			}


			struct control_parallel_asynch {
				typedef boost::optional<boost::system::error_code> err_code;
				std::vector<err_code> v;

				control_parallel_asynch() {}
				control_parallel_asynch(size_t n) : v(n, boost::none) {}

				struct task_is_terminated {
					bool operator() (const err_code& e) const {
						return e;
					}
				};

				struct task_is_success {
					bool operator() (const err_code& e) const{
						return e && (!*e);
					}
				};

				struct task_error {
					bool operator() (const err_code& e) const {
						return e && *e;
					}
				};


				bool is_termined() const {
					return hyper::utils::all(v.begin(), v.end(), task_is_terminated());
				};

				bool is_succes() const {
					return hyper::utils::all(v.begin(), v.end(), task_is_success());
				};

				boost::system::error_code first_error() const 
				{
					std::vector<err_code>::const_iterator it;
					it = std::find_if(v.begin(), v.end(), task_error());
					assert (it != v.end());
					return *(*it);
				}
			};

			typedef boost::shared_ptr<control_parallel_asynch> control_parallel_asynch_ptr;

			template <typename Handler>
			void handle_async_parallel_for_each(const boost::system::error_code& e, 
											    control_parallel_asynch_ptr ctrl,
												size_t n, boost::tuple<Handler> handler)
			{
				ctrl->v[n] = e;
				if (ctrl->is_termined()) {
					if (ctrl->is_succes())
						boost::get<0>(handler) (boost::system::error_code());
					else
						boost::get<0>(handler) (ctrl->first_error());
				}
			}
		}

		/*
		 * asynchronous sequential execution of a function on the sequence. 
		 * Iterator must model InputIterator
		 * Func is a binary function taking an argument T and callback handler
		 * Handler is an unary function taking in argument a boost::system::error_code.
		 *
		 * for a sequence like  [a, b, c]
		 * async_for_each(.., .., f, cb) will execute
		 *    f(a, cb_) -> cb_ -> f(b, cb_) -> cb_ -> f(c, cb_) -> cb_ -> cb
		 */
		template <typename It, typename Func, typename Handler>
		void async_for_each(It begin, It end, Func func, Handler handler)
		{
			if (std::distance(begin, end) == 0) 
				return handler(boost::system::error_code());

			void (*f)(const boost::system::error_code& e,
					  It begin, It end, Func func, boost::tuple<Handler>) =
				&details::handle_async_for_each<It, Func, Handler>;

			func(*begin, boost::bind(f, boost::asio::placeholders::error,
										begin, end, func, boost::make_tuple(handler)));
		}

		/* Asynchronous parallel execution of a function on a sequence.
		 * Iterator must model InputIterator
		 * Func is a binary function taking an argument T and callback handler
		 * Handler is an unary function taking in argument a boost::system::error_code.
		 *
		 * func is applied on each member on the sequence in parallel, but the
		 * handler is called only when each function has terminated. In case of
		 * multiple error, it returns a random one.
		 *
		 * The algorithm allocates a control vector (under its responsability).
		 */
		template <typename It, typename Func, typename Handler>
		void async_parallel_for_each(It begin, It end, Func func, Handler handler)
		{
			if (std::distance(begin, end) == 0) 
				return handler(boost::system::error_code());

			details::control_parallel_asynch_ptr c = 
				boost::make_shared<details::control_parallel_asynch>(std::distance(begin, end));

			void (*f) (const boost::system::error_code& e, 
					   details::control_parallel_asynch_ptr ctrl,
					   size_t n, boost::tuple<Handler>) =
				&details::handle_async_parallel_for_each<Handler>;

			size_t n = 0;
			for (; begin != end; n++, ++begin) 
				func(*begin, boost::bind(f, boost::asio::placeholders::error, c, n, boost::make_tuple(handler)));
		}
	}
}

#endif /* HYPER_NETWORK_ALGORITHM_HH_ */
