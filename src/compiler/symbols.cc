
#include <compiler/symbols.hh>

#include <algorithm>

#include <boost/bind.hpp>

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

