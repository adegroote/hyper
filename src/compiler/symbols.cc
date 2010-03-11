
#include <compiler/symbols.hh>

using namespace hyper::compiler;
using std::make_pair;

std::pair<bool, symbolList::symbolAddError> 
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

std::pair< bool, symbol > 
symbolList::get(const std::string &name) const
{
	sMap::const_iterator it = symbols.find(name);
	if (it == symbols.end())
		return make_pair(false, symbol());

	return make_pair(true, it->second);
}

