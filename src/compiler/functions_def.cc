
#include <compiler/functions_def.hh>

using namespace hyper::compiler;
using std::make_pair;
using boost::make_tuple;

boost::tuple< bool, functionDefList::addErrorType, int>
functionDefList::add(const std::string &name, 
						  const std::string &return_name,
						  const std::vector < std::string > & argsName)
{
	fmap::const_iterator it = functionDefs.find(name);
	if (it != functionDefs.end())
		return make_tuple(false, alreadyExist, 0);

	std::pair < bool, typeList::typeId > preturn;
	preturn = tlist.getId(return_name);
	if (preturn.first == false) 
		return make_tuple(false, unknowReturnType, 0);
	typeList::typeId returnTypeId = preturn.second;


	std::vector < typeList::typeId > v;
	for (size_t i = 0; i < argsName.size(); ++i)
	{
		std::pair < bool, typeList::typeId> p;
		p = tlist.getId(argsName[i]);
		if (p.first == false)
			return make_tuple(false, unknowArgsType, i);
		v.push_back(p.second);
	}

	functionDefs[name] = functionDef(name, v, returnTypeId);
	return make_tuple(true, noError, 0);
}

std::pair <bool, functionDef> 
functionDefList::get(const std::string &name) const
{
	fmap::const_iterator it = functionDefs.find(name);
	if (it == functionDefs.end())
		return make_pair(false, functionDef());

	return make_pair(true, it->second);
}
