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

std::ostream& operator << (std::ostream& os, const pipo& p)
{
	os << p.dummy << " " << p.my_d << " " << p.name << std::endl;
	return os;
}

bool operator == (const pipo& p1, const pipo& p2) 
{
	return p1.dummy == p2.dummy && p1.my_d == p2.my_d && p1.name == p2.name;
}

std::ostream& operator << (std::ostream& os, const complex_pipo& p)
{
	os << p.id << " " << p.pipo1 << " " << p.pipo2 << std::endl;
	return os;
}

bool operator == (const complex_pipo& p1, const complex_pipo& p2)
{
	return p1.id == p2.id && p1.pipo1 == p2.pipo1 && p1.pipo2 == p2.pipo2;
}


template <typename T>
void try_oarchive_interface(const T& src, const std::string& expected)
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

template <typename T>
void try_iarchive_interface(const std::string& input, const T& expected)
{
	T value;
	std::istringstream iss(input);
	hyper::network::json_iarchive ia(iss);

	ia >> value;

#if 0
	std::cerr << value << " : " << expected << std::endl;
#endif

	BOOST_CHECK(value  == expected);
}


BOOST_AUTO_TEST_CASE ( network_oarchive_json_test )
{
	bool b = true;
	try_oarchive_interface(b, "true");
	b = false;
	try_oarchive_interface(b, "false");

	int i = 22;
	try_oarchive_interface(i, "22");
	i = 42;
	try_oarchive_interface(i, "42");

	double d = 33.15;
	try_oarchive_interface(d, "33.15");
	d = 77.17;
	try_oarchive_interface(d, "77.17");

	std::string s = "pipo";
	try_oarchive_interface(s, "\"pipo\"");
	s = "xxx";
	try_oarchive_interface(s, "\"xxx\"");

	pipo p;
	p.dummy = 42;
	p.my_d = 3.1415;
	p.name = "cnrs";

	try_oarchive_interface(p, "{ \"dummy\": 42, \"my_d\": 3.1415, \"name\": \"cnrs\" }");

	complex_pipo cp;
	cp.id = 30;
	cp.pipo1 = p;
	cp.pipo2.dummy = 666;
	cp.pipo2.my_d = 2.718;
	cp.pipo2.name = "laas";

	try_oarchive_interface(cp, "{ \"id\": 30, \"pipo1\": { \"dummy\": 42, \"my_d\": 3.1415, \"name\": \"cnrs\" }, "
											"\"pipo2\": { \"dummy\": 666, \"my_d\": 2.718, \"name\": \"laas\" } }");

	boost::optional<double> od = boost::none;

	try_oarchive_interface(od, "null");
	od = 1.618;
	try_oarchive_interface(od, "1.618");

	boost::optional<bool> ob = boost::none;
	try_oarchive_interface(ob, "null");
	ob = false;
	try_oarchive_interface(ob, "false");
}


BOOST_AUTO_TEST_CASE ( network_iarchive_json_test )
{
	try_iarchive_interface("true", true);
	try_iarchive_interface("  false", false);

	try_iarchive_interface("22", 22);
	try_iarchive_interface("42", 42);

	try_iarchive_interface("33.15", 33.15);
	try_iarchive_interface("77.17", 77.17);

	try_iarchive_interface("\"pipo\"", std::string("pipo"));
	try_iarchive_interface("\"xxx\"", std::string("xxx"));
	try_iarchive_interface("\"\"", std::string(""));

	pipo p;
	p.dummy = 42;
	p.my_d = 3.1415;
	p.name = "cnrs";

	try_iarchive_interface("{ \"dummy\": 42, \"my_d\": 3.1415, \"name\": \"cnrs\" }", p);

	complex_pipo cp;
	cp.id = 30;
	cp.pipo1 = p;
	cp.pipo2.dummy = 666;
	cp.pipo2.my_d = 2.718;
	cp.pipo2.name = "laas";

	try_iarchive_interface("{ \"id\": 30, \"pipo1\": { \"dummy\": 42, \"my_d\": 3.1415, \"name\": \"cnrs\" }, "
										 "\"pipo2\": { \"dummy\": 666, \"my_d\": 2.718, \"name\": \"laas\" } }", cp);

	boost::optional<double> od = boost::none;

	try_iarchive_interface("null", od);
	od = 1.618;
	try_iarchive_interface("1.618", od);
 
	boost::optional<bool> ob = boost::none;
	try_iarchive_interface("null", ob);
	ob = false;
	try_iarchive_interface("false", ob);
	ob = true;
	try_iarchive_interface("true", ob);
}
