#include <sstream>
#include <algorithm>

#include <boost/bind.hpp>

#include <compiler/symbols.hh>

using namespace hyper::compiler;
using std::make_pair;

symbolList::add_result
symbolList::add(const std::string &name, const std::string& tname)
{
	sMap::const_iterator it = symbols.find(name);
	if (it != symbols.end())
		return make_pair(false, alreadyExist);

	std::pair < bool, typeList::typeId> p;
	p = tlist.getId(tname);
	if (p.first == false)
		return make_pair(false, unknowType);

	symbols[name] = symbol(name, p.second);
	return make_pair(true, noError);
}

symbolList::add_result
symbolList::add(const symbol_decl &s)
{
	return add(s.name, s.typeName);
}

std::vector<symbolList::add_result>
symbolList::add(const symbol_decl_list &list)
{
	std::vector<symbolList::add_result> res(list.l.size());

	// help the compiler to find the right overloading
	symbolList::add_result (symbolList::*add) (const symbol_decl&) = &symbolList::add;

	std::transform(list.l.begin(), list.l.end(), res.begin(),
				   boost::bind(add, this, _1));
				   
	return res;
}

std::pair< bool, symbol > 
symbolList::get(const std::string &name) const
{
	sMap::const_iterator it = symbols.find(name);
	if (it == symbols.end())
		return make_pair(false, symbol());

	return make_pair(true, it->second);
}

std::string
symbolList::get_diagnostic(const symbol_decl& decl, const symbolList::add_result& res)
{
	if (res.first)
		return "";

	std::ostringstream oss;
	switch (res.second)
	{
		case symbolList::alreadyExist:
			oss << "symbol " << decl.name << " is already defined " << std::endl;
			break;
		case symbolList::unknowType:
			oss << "type " << decl.typeName << " used to define " << decl.name;
			oss << " is not defined " << std::endl;
			break;
		case symbolList::noError:
		default:
			assert(false);
	};

	return oss.str();
}

std::vector<std::string>
symbolList::intersection(const symbolList &s) const
{
	std::vector<std::string> s1, s2, s_res;
	sMap::const_iterator it;

	for (it = symbols.begin(); it != symbols.end(); ++it)
		s1.push_back(it->first);
	for (it = s.symbols.begin(); it != s.symbols.end(); ++it)
		s2.push_back(it->first);

	std::sort(s1.begin(), s1.end());
	std::sort(s2.begin(), s2.end());

	std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), std::back_inserter(s_res));

	return s_res;
}


