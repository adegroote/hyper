#include <cassert>
#include <algorithm>

#include <boost/bind.hpp>

#include <compiler/symbols.hh>
#include <compiler/types.hh>

using namespace hyper::compiler;

typeList::add_result
typeList::add(const std::string& name, typeOfType t)
{
	typeList::native_decl_error error;
	typeMap::const_iterator it = mapsId.find(name);
	if (it != mapsId.end()) {
		error.err = typeList::typeAlreadyExist;
		return boost::make_tuple(false, it->second, error);
	}

	types.push_back(type(name, t));
	mapsId[name] = types.size() - 1;
	error.err = typeList::noError;
	return boost::make_tuple(true, types.size() - 1, error);
}

struct add_type : public boost::static_visitor < typeList::add_result >
{
	typeList & tList;

	add_type(typeList & tList_) : tList(tList_) {};

	typeList::add_result operator() (const Nothing & decl)
	{
		assert(false);
	}

	typeList::add_result operator() (const struct_decl& decl)
	{
		typeList::struct_decl_error error;
		error.local_err = typeList::notTested;

		boost::shared_ptr<symbolList> s(new symbolList(tList));
		std::vector < symbolList::add_result > s_result;
		s_result = s->add(decl.vars);

		std::vector < symbolList::add_result >::const_iterator it; 
		bool ok = true;
		for (it = s_result.begin(); it != s_result.end(); ++it)
		{
			ok = ok && it->first;
			switch (it->second) {
				case symbolList::noError:
					error.var_err.push_back(typeList::noError);
					break;
				case symbolList::alreadyExist:
					error.var_err.push_back(typeList::symAlreadyExist);
					break;
				case symbolList::unknowType:
					error.var_err.push_back(typeList::unknowType);
					break;
			};
		}
		if (ok == false)
			return boost::make_tuple(false, 0, error);

		typeList::add_result r = tList.add(decl.name, structType);
		if (r.get<0>() == false) {
			error.local_err = typeList::symAlreadyExist;
			r.get<2>() = error;
			return r;
		} 

		error.local_err = typeList::noError;
		type& t = tList.get(r.get<1>());
		t.internal = s;

		r.get<2>() = error;
		return r;
	}

	typeList::add_result operator() (const newtype_decl& decl)
	{
		typeList::new_decl_error error;
		error.new_name_error = typeList::notTested;

		std::pair < bool, typeList::typeId> ref = tList.getId(decl.oldname);
		if (ref.first == false) {
			error.old_name_error = typeList::unknowType;
			return boost::make_tuple(false, 0, error);
		};

		error.old_name_error = typeList::noError;

		typeList::add_result r = tList.add(decl.newname, noType);
		if (r.get<0>() == false) {
			error.new_name_error = typeList::typeAlreadyExist;
			r.get<2>() = error;
			return r;
		}

		type& t = tList.get(r.get<1>());
		type type_ref = tList.get(ref.second);
		
		t.t = type_ref.t;
		t.internal = ref.second;

		error.new_name_error = typeList::noError;
		r.get<2>() = error;
		return r;
	}
};

typeList::add_result
typeList::add(const type_decl& decl)
{
	add_type add(*this);
	return boost::apply_visitor(add, decl);
}

std::vector < typeList::add_result >
typeList::add(const type_decl_list& list)
{
	std::vector<typeList::add_result> res(list.l.size());

	// help the compiler to find the right overloading
	typeList::add_result (typeList::*add) (const type_decl&) =
	   	&typeList::add;

	std::transform(list.l.begin(), list.l.end(), res.begin(),
				   boost::bind(add, this, _1));

	return res;
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

type& typeList::get(typeList::typeId t)
{
	assert(t < types.size());
	return types[t];
}
	

