#ifndef HYPER_NETWORK_JSON_ARCHIVE_HPP_
#define HYPER_NETWORK_JSON_ARCHIVE_HPP_

#include <ostream>
#include <cstddef> 

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/optional/optional.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/optional.hpp>


namespace hyper {
	namespace network {
		class json_oarchive {
			std::ostream & m_os;
			bool need_comma;

			template<class Archive>
			struct save_enum_type 
			{
				template<typename T>
				static void invoke(Archive &ar, const T &t){
					ar.m_os << static_cast<int>(t);
				}
			};

			template<class Archive>
			struct save_primitive 
			{
				template<typename T>
				static void invoke(Archive & ar, const T & t){
					ar.m_os << t;
				}
			};

			template<class Archive>
			struct save_only {
				template<typename T>
				static void invoke(Archive & ar, const T & t){
					ar.m_os << "{ ";
					ar.need_comma = false;
					boost::serialization::serialize_adl(
						ar, 
						const_cast<T &>(t), 
						::boost::serialization::version< T >::value
					);
					ar.need_comma = false;
					ar.m_os << " }";
				}
			};

			template<typename T>
			void save(const T &t)
			{
				typedef 
					BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<boost::is_enum< T >,
						boost::mpl::identity<save_enum_type<json_oarchive> >,
					//else
					BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<
						// if its primitive
							boost::mpl::equal_to<
								boost::serialization::implementation_level< T >,
								boost::mpl::int_<boost::serialization::primitive_type>
							>,
							boost::mpl::identity<save_primitive<json_oarchive> >,
					// else
						boost::mpl::identity<save_only<json_oarchive> >
					> >::type typex;
				typex::invoke(*this, t);
			}    

		public:
			///////////////////////////////////////////////////
			// Implement requirements for archive concept

			typedef boost::mpl::bool_<false> is_loading;
			typedef boost::mpl::bool_<true> is_saving;

			// this can be a no-op since we ignore pointer polymorphism
			template<typename T>
			void register_type(const T * = NULL){}

			unsigned int get_library_version()
			{
				return 0;
			}

			void 
			save_binary(const void *address, std::size_t count)
			{
				m_os << "save_binary not implemented";
			}

			// the << operators 
			template<typename T>
			json_oarchive & operator<< (const T& t)
			{
				save(t);
				return *this;
			}

			json_oarchive & operator<< (bool b) 
			{
				if (b)
					m_os << "true";
				else
					m_os << "false";
				return *this;
			}

			json_oarchive & operator<< (const std::string& s)
			{
				m_os << "\"" << s << "\"";
				return *this;
			}

			template <typename T>
			json_oarchive & operator<< (const boost::optional<T>& t) 
			{
				if (!t)
					m_os << "null";
				else
					*this << *t;
				return *this;
			}

			template<typename T>
			json_oarchive & operator<< (const T *t)
			{
				if(t == NULL)
					m_os << "null";
				else
					*this << *t;
				return *this;
			}

			template<typename T, int N>
			json_oarchive & operator<<(const T (&t)[N])
			{
				return *this << boost::serialization::make_array(
					static_cast<const T *>(&t[0]),
					N
				);
			}

			template<typename T>
			json_oarchive & operator<<(const boost::serialization::nvp< T > & t)
			{
				if (need_comma) 
					m_os << ", ";
				m_os << "\"" << t.name() << "\": "; // output the name of the object
				*this << t.const_value();
				need_comma = true;
				return *this;
			}

			// the & operator 
			template<typename T>
			json_oarchive & operator&(const T & t)
			{
					return *this << t;
			}
			///////////////////////////////////////////////

			json_oarchive(std::ostream & os) :
				m_os(os)
			{}
		};

		class json_iarchive {
			/* A simple json archive input 
			 *
			 * The input is decoded using basically a property_tree @pt, and a
			 * json_parser::read_json call. But it works only at the 'object'
			 * level of JSon, not raw data, like double or int. For this
			 * special case, just use a copy of the input in @content, and work
			 * on it as a istringstream. 
			 * 
			 * @path is used to determine at which object level we are in the
			 * hierarchy. If path is empty, it means we read 'raw data', and so
			 * just use @content.
			 *
			 * The implementation may be simpler with a real 'json parser', but
			 * it would add some dependencies to the code.
			 */
			boost::property_tree::ptree pt;
			std::string content;
			std::vector<std::string> path;

			template<class Archive>
			struct load_enum_type 
			{
				template<typename T>
				static void invoke(Archive &ar, T &t){
					if (ar.path.empty()) {
						std::istringstream iss(ar.content);
						int value;
						iss >> value;
						t = static_cast<T>(value);
					} else {}
				}
			};

			template<class Archive>
			struct load_primitive 
			{
				template<typename T>
				static void invoke(Archive & ar, T & t){
					if (ar.path.empty()) {
						std::istringstream iss(ar.content);
						iss >> t;
					} else {
						t = ar.pt.template get<T>(boost::algorithm::join(ar.path, "."));
					}
				}
			};

			template<class Archive>
			struct load_only {
				template<typename T>
				static void invoke(Archive & ar, T & t){
					ar.path.push_back("");
					boost::serialization::serialize_adl(
						ar, 
						t, 
						::boost::serialization::version< T >::value
					);
					ar.path.pop_back();
				}
			};

			template<typename T>
			void load(T &t)
			{
				typedef 
					BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<boost::is_enum< T >,
						boost::mpl::identity<load_enum_type<json_iarchive> >,
					//else
					BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<
						// if its primitive
							boost::mpl::equal_to<
								boost::serialization::implementation_level< T >,
								boost::mpl::int_<boost::serialization::primitive_type>
							>,
							boost::mpl::identity<load_primitive<json_iarchive> >,
					// else
						boost::mpl::identity<load_only<json_iarchive> >
					> >::type typex;
				typex::invoke(*this, t);
			}    

		public:
			///////////////////////////////////////////////////
			// Implement requirements for archive concept

			typedef boost::mpl::bool_<true> is_loading;
			typedef boost::mpl::bool_<false> is_saving;

			// this can be a no-op since we ignore pointer polymorphism
			template<typename T>
			void register_type(const T * = NULL){}

			unsigned int get_library_version()
			{
				return 0;
			}

			void 
			load_binary(void *address, std::size_t& count)
			{
				throw boost::archive::archive_exception(
						 boost::archive::archive_exception::unregistered_cast);
			}

			// the >> operators 
			template<typename T>
			json_iarchive & operator>> (T& t)
			{
				load(t);
				return *this;
			}

			json_iarchive & operator>> (bool& b) 
			{
				if (path.empty()) {
					if (content == "true")
						b = true;
					else if (content == "false")
						b = false;
					else
						throw boost::archive::archive_exception(
								 boost::archive::archive_exception::unregistered_cast);
				} else {}

				return *this;
			}

			json_iarchive & operator>> (std::string& s)
			{
				if (path.empty()) {
					assert(content.size() >= 2);
					s = content.substr(1, content.size() - 2);
				} else {
					s = pt.get<std::string>(boost::algorithm::join(path, "."));
				}

				return *this;
			}

			template <typename T>
			json_iarchive & operator>> (boost::optional<T>& t) 
			{
				std::string s;
				if (path.empty()) {
					s = content;
				} else {
					s = pt.get<std::string>(boost::algorithm::join(path, "."));
				}

				if (s == "null") {
					t = boost::none;
				} else { 
					T value;
					*this >> value;
					t = value;
				}

				return *this;
			}

			template<typename T>
			json_iarchive & operator>> (T *t)
			{
				std::string s;
				if (path.empty()) {
					s = content;
				} else {
					s = pt.get<std::string>(boost::algorithm::join(path, "."));
				}

				if (s == "null") {
					t = 0;
				} else { 
					*this >> *t;
				}

				return *this;
			}

			template<typename T, int N>
			json_iarchive & operator>>(T (&t)[N])
			{
				return *this >> boost::serialization::make_array(
					static_cast<const T *>(&t[0]),
					N
				);
			}

			template<typename T>
			json_iarchive& operator>>(const boost::serialization::nvp< T > & t)
			{
				path.back() = t.name();
				*this >> t.value();
				return *this;
			}

			// the & operator 
			template<typename T>
			json_iarchive & operator&(T & t)
			{
					return *this >> t;
			}
			///////////////////////////////////////////////

			json_iarchive(std::istream & is)
			{
				std::istreambuf_iterator<char> eos;
				content = boost::trim_copy(std::string(std::istreambuf_iterator<char>(is), eos));
				try {
					std::istringstream iss(content);
					boost::property_tree::json_parser::read_json(iss, pt);
				} catch (std::runtime_error&) {}
			}
		};
	}
}


#endif // HYPER_NETWORK_JSON_ARCHIVE_HPP_
