#include <algorithm>

#include <boost/bind.hpp>

#include <compiler/functions_def.hh>
#include <compiler/scope.hh>

using namespace hyper::compiler;
using std::make_pair;
using boost::make_tuple;

void 
functionDef::output_proto(std::ostream& oss, const typeList& tList) const
{
	type ret = tList.get(returnType());
	oss << "\t\t" << ret.name << " " << scope::get_identifier(name()) << "(";
	for (size_t i = 0; i < arity(); ++i) 
	{
		type arg = tList.get(argsType(i));
		oss << arg.name;
		if (arg.t == stringType || arg.t == structType)
			oss << " const & ";
		if (i != arity() - 1) 
			oss << ", ";
	}

	oss << " );" << std::endl;
}

void 
functionDef::output_impl(std::ostream& oss, const typeList& tList) const
{
	type ret = tList.get(returnType());
	oss << "\t\t" << ret.name << " " << scope::get_identifier(name()) << "(";
	for (size_t i = 0; i < arity(); ++i) 
	{
		type arg = tList.get(argsType(i));
		oss << arg.name;
		if (arg.t == stringType || arg.t == structType)
			oss << " const & ";
		oss << "v" << i;
		if (i != arity() - 1) 
			oss << ", ";
	}

	oss << " )" << std::endl;
	oss << "\t\t{\n\t\t}" << std::endl;
}

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
std::string			    
functionDefList::get_diagnostic(const function_decl& decl, const add_result& res)
{
	if (res.get<0>())
		return "";
	
	std::ostringstream oss;
	switch (res.get<1>()) {
		case functionDefList::alreadyExist:
			oss << decl.fName << " is already defined " << std::endl;
			break;
		case functionDefList::unknowReturnType:
			oss << "type " << decl.returnName << " used in definition of ";
			oss << decl.fName << " is not defined " << std::endl;
			break;
		case functionDefList::unknowArgsType:
			oss << "type " << decl.argsName[res.get<2>()];
			oss << " used in definition of " << decl.fName << " is not defined ";
			oss << std::endl;
			break;
		case functionDefList::noError:
		default:
			assert(false);
	}
	return oss.str();
}
