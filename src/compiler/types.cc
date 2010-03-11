#include <compiler/types.hh>
#include <cassert>

using namespace hyper::compiler;

std::pair <bool, typeList::typeId> typeList::add(const std::string& name, typeOfType t)
{
	typeMap::const_iterator it = mapsId.find(name);
	if (it != mapsId.end())
		return std::make_pair(false, it->second);

	types.push_back(type(name, t));
	mapsId[name] = types.size() - 1;
	return std::make_pair(true, types.size() - 1);
}

std::pair <bool, typeList::typeId> typeList::getId(const std::string &name) const
{
	typeMap::const_iterator it = mapsId.find(name);
	if (it == mapsId.end())
		return std::make_pair(false, 0);
	return std::make_pair(true, it->second);
}

type typeList::get(typeList::typeId t) const
{
	assert(t < types.size());
	return types[t];
}
