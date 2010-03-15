#include <algorithm>

#include <boost/bind.hpp>

#include <compiler/functions_def.hh>

using namespace hyper::compiler;
using std::make_pair;
using boost::make_tuple;

functionDefList::add_result
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

functionDefList::add_result
functionDefList::add(const function_decl& decl)
{
	return add(decl.fName, decl.returnName, decl.argsName);
}

std::vector<functionDefList::add_result>
functionDefList::add(const function_decl_list& list)
{
	std::vector<functionDefList::add_result> res(list.l.size());

	// help the compiler to find the right overloading
	functionDefList::add_result (functionDefList::*add) (const function_decl&) =
	   	&functionDefList::add;

	std::transform(list.l.begin(), list.l.end(), res.begin(),
				   boost::bind(add, this, _1));
				   
	return res;
}

std::pair <bool, functionDef> 
functionDefList::get(const std::string &name) const
{
	fmap::const_iterator it = functionDefs.find(name);
	if (it == functionDefs.end())
		return make_pair(false, functionDef());

	return make_pair(true, it->second);
}
