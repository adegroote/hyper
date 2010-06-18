
#ifndef _HYPER_NETWORK_PROXY_HH_
#define _HYPER_NETWORK_PROXY_HH_

#include <network/client_tcp_impl.hh>
#include <network/nameserver.hh>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/function.hpp>
#include <boost/optional/optional.hpp>
#include <boost/none.hpp>

#include <cassert>

namespace hyper {
	namespace network {

		template <typename T, bool is_local = true> struct proxy {};

		template <typename T>
		struct proxy<T, true> 
		{
			T& value_;

			proxy(T& value) : value_(value) {};

			const T& operator() () const { return value_;}
		};

		template <typename T>
		std::string serialize_value(const T& value)
		{
			std::ostringstream archive_stream;
			boost::archive::text_oarchive archive(archive_stream);
			archive << value;
			return archive_stream.str();
		}

		template <typename T>
		boost::optional<T> deserialize_value(const std::string& value)
		{
			T res;
			try {
				std::istringstream archive_stream(value);
				boost::archive::text_iarchive archive(archive_stream);
				archive >> res;
				return res;
			} catch (std::exception& e) {
				return boost::optional<T>();
			}
		}

		class proxy_serializer
		{
			typedef std::map<std::string, boost::function <std::string ()> > serializer;
			serializer s;
			mutable boost::shared_mutex m_;

			public:
				proxy_serializer() {};
				template <typename T>
				bool register_variable(const std::string& name, const T& value)
				{
					std::string(*f) (const T& value) = &serialize_value<T>;
					std::pair< serializer::iterator, bool > p;

					boost::upgrade_lock<boost::shared_mutex> lock(m_);
					p = s.insert(std::make_pair(name, 
								boost::bind(f, boost::cref(value))));
					return p.second;
				}

				std::pair<bool, std::string> eval(const std::string& name) const
				{
					serializer::const_iterator it;
					boost::shared_lock<boost::shared_mutex> lock(m_);
					it = s.find(name);
					if (it == s.end()) 
						return std::make_pair(false, "");
					return std::make_pair(true, it->second());
				}
		};

		typedef boost::mpl::vector< request_variable_value > proxy_input_msg;
		typedef boost::mpl::vector< variable_value > proxy_output_msg;
		typedef boost::make_variant_over< proxy_input_msg>::type proxy_input_variant;
		typedef boost::make_variant_over< proxy_output_msg>::type proxy_output_variant;
	
		struct proxy_visitor : public boost::static_visitor<proxy_output_variant>
		{
			proxy_serializer& s;
	
			proxy_visitor(proxy_serializer& s_) : s(s_) {};
			
			proxy_output_variant operator() (const request_variable_value& r) const
			{
				std::pair<bool, std::string> p = s.eval(r.var_name);
				
				variable_value res;
				res.var_name = r.var_name;
				res.success = p.first;
				res.value = p.second;
	
				return res;
			}
		};

		/*
		 * In the remote case, @T must be serializable. In the remote case
		 * proxy, the proxy object is a consummer of a remote publised
		 * information. 
		 */
		template <typename T>
		struct proxy<T, false>
		{
			boost::optional<T> value_;

			proxy(boost::asio::io_service & io_s) { assert(false); };

			const boost::optional<T>& operator() () const { return value_; };
		};


	};
};

#endif /* _HYPER_NETWORK_PROXY_HH_ */
