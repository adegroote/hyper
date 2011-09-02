#ifndef HYPER_NETWORK_PROXY_CONTAINER_HH_
#define HYPER_NETWORK_PROXY_CONTAINER_HH_

#include <map>
#include <sstream>
#include <string>

#include <boost/any.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/function/function0.hpp>
#include <boost/optional/optional.hpp>

namespace hyper {
	namespace network {

		template <typename T>
		struct proxy
		{
			T& value_;

			proxy(T& value) : value_(value) {};

			const T& operator() () const { return value_;}
		};

		template <typename T>
		std::string serialize_value(const T& value)
		{
			std::ostringstream archive_stream;
			boost::archive::binary_oarchive archive(archive_stream);
			archive << value;
			return archive_stream.str();
		}

		template <typename T>
		boost::optional<T> deserialize_value(const std::string& value)
		{
			T res;
			try {
				std::istringstream archive_stream(value);
				boost::archive::binary_iarchive archive(archive_stream);
				archive >> res;
				return res;
			} catch (std::exception& ) {
				return boost::optional<T>();
			}
		}

		template <typename T>
		boost::any capture_value(const T& value)
		{
			return value;
		}

		class proxy_serializer
		{
			typedef std::map<std::string, boost::function <std::string ()> > serializer;
			serializer s;

			public:
				proxy_serializer() {};
				template <typename T>
				bool register_variable(const std::string& name, const T& value)
				{
					std::string(*f) (const T& value) = &serialize_value<T>;
					std::pair< serializer::iterator, bool > p;

					p = s.insert(std::make_pair(name, 
								boost::bind(f, boost::cref(value))));
					return p.second;
				}

				std::pair<bool, std::string> eval(const std::string& name) const
				{
					serializer::const_iterator it;
					it = s.find(name);
					if (it == s.end()) 
						return std::make_pair(false, "");
					return std::make_pair(true, it->second());
				}

				bool remove_variable(const std::string& name)
				{
					serializer::iterator it = s.find(name);
					if (it == s.end())
						return false;
					else {
						s.erase(it);
						return true;
					}
				}
		};

		class local_proxy 
		{
			typedef std::map<std::string, boost::function<boost::any ()> > l_proxy;
			l_proxy l;

			public:
				local_proxy() {};
				template <typename T>
				bool register_variable(const std::string& name, const T& value)
				{
					boost::any(*f) (const T& value) = &capture_value<T>;
					std::pair< l_proxy::iterator, bool > p;

					p = l.insert(std::make_pair(name, 
								boost::bind(f, boost::cref(value))));
					return p.second;
				}

				/* 
				 * It will be nice to pass a boost::system::error to know the
				 * failure cause
				 */
				template <typename T>
				boost::optional<T> eval(const std::string& name) const
				{
					l_proxy::const_iterator it;
					it = l.find(name);
					if (it == l.end()) 
						return boost::none;
					try { 
						T value = boost::any_cast<T>(it->second());
						return value;
					} catch (const boost::bad_any_cast &) {
						return boost::none;
					}

					return boost::none;
				}

				bool remove_variable(const std::string& name)
				{
					l_proxy::iterator it = l.find(name);
					if (it == l.end())
						return false;
					else {
						l.erase(it);
						return true;
					}
				}
		};
	}
}

#endif /* HYPER_NETWORK_PROXY_CONTAINER_HH_ */
