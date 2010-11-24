#ifndef _HYPER_MODEL_TASK_HH_
#define _HYPER_MODEL_TASK_HH_

#include <network/nameserver.hh>
#include <network/proxy.hh>

#include <logic/expression.hh>

#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/shared_ptr.hpp>

namespace hyper {
	namespace model {

		struct ability;


		class task
		{
			private:
				const ability& a;
				std::string name;

			public:
				task(const ability& a_, const std::string& name_) 
					: a(a_), name(name_) {}

				virtual ~task() {};
		};

		typedef boost::shared_ptr<task> task_ptr;

		/*
		 * Expression will be sub-classed to build real computational
		 * expression on top of a specification 
		 * */

		template <typename vectorT>
		class expression
		{
			public:
			typedef network::remote_proxy_n<vectorT, network::name_client> r_proxy_n;
			typedef boost::array<std::pair<std::string, std::string>, r_proxy_n::size> arg_list;
	
			r_proxy_n r_proxy;

			expression(boost::asio::io_service& io_s,
						    const arg_list& l,
							network::name_client &r) :
				r_proxy(io_s, l, r) {}
			;
	
			private:
			template <typename Handler>
			void handle_proxy_get(boost::tuple<Handler> handler)
			{
				bool res = r_proxy.is_successful();
				if (res) 
					res = real_compute();
				boost::get<0>(handler)(res);
			}
	
			public:
			template <typename Handler>
			void compute(Handler handler)
			{
				void (expression::*f) (boost::tuple<Handler>)
					= &expression::template handle_proxy_get<Handler>;
				r_proxy.async_get(boost::bind(f, boost::make_tuple(handler)));
			}

			protected:
			virtual bool real_compute() const { return false; }
		};

		template <>
		struct expression<boost::mpl::vector<> >
		{
			public:
			expression() {}
	
			template <typename Handler>
			void compute(Handler handler)
			{
				bool res = real_compute();
				handler(res);
			}

			protected:
			virtual bool real_compute() const { return false; };
		};
	}
}

#endif /* _HYPER_MODEL_TASK_HH_ */
