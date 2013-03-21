#include <network/json_archive.hh>
#include <boost/test/unit_test.hpp>
#include <boost/optional.hpp>

struct pipo {
	int dummy;
	double my_d;
	std::string name;

	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			(void) version;
			ar & BOOST_SERIALIZATION_NVP(dummy) & BOOST_SERIALIZATION_NVP(my_d) & BOOST_SERIALIZATION_NVP(name);
		}
};

struct complex_pipo {
	int id;
	pipo pipo1;
	pipo pipo2;
	private:
		friend class boost::serialization::access;
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			(void) version;
			ar & BOOST_SERIALIZATION_NVP(id) & BOOST_SERIALIZATION_NVP(pipo1) & BOOST_SERIALIZATION_NVP(pipo2);
		}
};



template <typename T>
void try_archive_interface(const T& src, const std::string& expected)
{
	std::ostringstream oss;
	hyper::network::json_oarchive oa(oss);

	oa << src;

#if 0
	std::cerr << oss.str() <<  std::endl;
	std::cerr << expected << std::endl;
#endif

	BOOST_CHECK(oss.str() == expected);
}

BOOST_AUTO_TEST_CASE ( network_archive_json_test )
{
	bool b = true;
	try_archive_interface(b, "true");
	b = false;
	try_archive_interface(b, "false");

	int i = 22;
	try_archive_interface(i, "22");
	i = 42;
	try_archive_interface(i, "42");

	double d = 33.15;
	try_archive_interface(d, "33.15");
	d = 77.17;
	try_archive_interface(d, "77.17");

	std::string s = "pipo";
	try_archive_interface(s, "\"pipo\"");
	s = "xxx";
	try_archive_interface(s, "\"xxx\"");

	pipo p;
	p.dummy = 42;
	p.my_d = 3.1415;
	p.name = "cnrs";

	try_archive_interface(p, "{ \"dummy\": 42, \"my_d\": 3.1415, \"name\": \"cnrs\" }");

	complex_pipo cp;
	cp.id = 30;
	cp.pipo1 = p;
	cp.pipo2.dummy = 666;
	cp.pipo2.my_d = 2.718;
	cp.pipo2.name = "laas";

	try_archive_interface(cp, "{ \"id\": 30, \"pipo1\": { \"dummy\": 42, \"my_d\": 3.1415, \"name\": \"cnrs\" }, "
											"\"pipo2\": { \"dummy\": 666, \"my_d\": 2.718, \"name\": \"laas\" } }");

	boost::optional<double> od = boost::none;

	try_archive_interface(od, "null");
	od = 1.618;
	try_archive_interface(od, "1.618");
}


