#include <hyperConfig.hh>

#include <algorithm>

#include <boost/bind.hpp>

#include <compiler/functions_def.hh>
#include <compiler/output.hh>
#include <compiler/scope.hh>
#include <compiler/types.hh>

using namespace hyper::compiler;
using std::make_pair;
using boost::make_tuple;

void 
functionDef::output_proto(std::ostream& oss, const typeList& tList) const
{
	oss << "\t\tstruct " << scope::get_identifier(name()) << " {" << std::endl;;
	type ret = tList.get(returnType());
	oss << "\t\t\t" << "typedef " << ret.type_name() << " result_type;" << std::endl;
	oss << "\t\t\t" << "typedef boost::mpl::vector<";
	for (size_t i = 0; i < arity(); ++i) 
	{
		type arg = tList.get(argsType(i));
		oss << arg.type_name();
		if (i != arity() - 1) 
			oss << ", ";
	}
	oss << "> args_type;" << std::endl;

	oss << "\t\t\tstatic " << ret.type_name() << " apply (";
	for (size_t i = 0; i < arity(); ++i) 
	{
		type arg = tList.get(argsType(i));
		oss << arg.type_name();
		if (arg.t == stringType || arg.t == structType || arg.t == opaqueType)
			oss << " const &";
		else
			oss << " ";
		if (i != arity() - 1) 
			oss << ", ";
	}

	oss << ");" << std::endl;
	oss << "\t\t};\n\n";
}

void 
functionDef::output_impl(std::ostream& oss, const typeList& tList) const
{
	type ret = tList.get(returnType());
	oss << "\t\t" << ret.type_name() << " " << scope::get_identifier(name()) << "::apply(";
	for (size_t i = 0; i < arity(); ++i) 
	{
		type arg = tList.get(argsType(i));
		oss << arg.type_name();
		if (arg.t == stringType || arg.t == structType || arg.t == opaqueType)
			oss << " const & ";
		else
			oss << " ";
		oss << "v" << i;
		if (i != arity() - 1) 
			oss << ", ";
	}

	oss << " )" << std::endl;
	oss << "\t\t{\n#error " << scope::get_identifier(name()) << " not implemented !!\n";
	oss << "\t\t}" << std::endl;
}

void
functionDef::output_import(std::ostream& oss, const typeList& tList) const
{
	std::pair<bool, typeId> p = tList.getId("bool");
	assert(p.first);
	bool is_predicate = (p.second == returnType());
	oss << "\t\t\t\ta.logic()." ;
	if (is_predicate) 
		oss << "add_predicate<";
	else
		oss << "add_func<" ;
	oss << name() << ">(" << quoted_string(name()) << ",\n";
	oss << "\tboost::assign::list_of";
	for (size_t i = 0; i < arity(); ++i) {
		type arg = tList.get(argsType(i));
		oss << "(" << quoted_string(arg.type_name()) << ")";
	}
	if (!is_predicate) {
		type ret = tList.get(returnType());
		oss << "(" << quoted_string(ret.type_name()) << ")";
	}
	oss << ");\n";
}

functionDefList::add_result
functionDefList::add(const std::string &name, 
						  const std::string &return_name,
						  const std::vector < std::string > & argsName,
						  const boost::optional<std::string>& tag)
{
	fmap::const_iterator it = functionDefs.find(name);
	if (it != functionDefs.end())
		return make_tuple(false, alreadyExist, 0);

	std::pair < bool, typeId > preturn;
	preturn = tlist.getId(return_name);
	if (preturn.first == false) 
		return make_tuple(false, unknowReturnType, 0);
	typeId returnTypeId = preturn.second;


	std::vector < typeId > v;
	for (size_t i = 0; i < argsName.size(); ++i)
	{
		std::pair < bool, typeId> p;
		p = tlist.getId(argsName[i]);
		if (p.first == false)
			return make_tuple(false, unknowArgsType, i);
		v.push_back(p.second);
	}

	functionDefs[name] = functionDef(name, v, returnTypeId, tag);
	return make_tuple(true, noError, 0);
}

functionDefList::add_result
functionDefList::add(const function_decl& decl)
{
	return add(decl.fName, decl.returnName, decl.argsName, decl.tag);
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
