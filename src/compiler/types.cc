#include <hyperConfig.hh>

#include <cassert>
#include <algorithm>

#include <boost/bind.hpp>

#include <compiler/output.hh>
#include <compiler/symbols.hh>
#include <compiler/types.hh>
#include <compiler/scope.hh>

using namespace hyper::compiler;

struct dump_struct
{
	std::ostream & oss;
	const typeList& tList;

	dump_struct(std::ostream& oss_, const typeList& tList_) : 
		oss(oss_), tList(tList_) {};

	void operator() (const std::pair<std::string, symbol>& p)
	{
		type t = tList.get(p.second.t);
		oss << "\t\t\t" << t.type_name() << " " << p.first << ";" << std::endl;
	}
};

struct dump_serialize_struct
{
	std::ostream & oss;

	dump_serialize_struct(std::ostream& oss_) : oss(oss_) {};

	void operator() (const std::pair<std::string, symbol>& p)
	{
		oss << " & " << p.first;
	}
};

struct default_construct
{
	std::ostream& oss;
	const typeList& tList;

	default_construct(std::ostream& oss, const typeList& tList) :
		oss(oss), tList(tList)
	{}

	void operator() (const std::pair<std::string, symbol>& p)
	{
		type t = tList.get(p.second.t);
		switch (t.t) {
			case intType:
				oss << p.first << "(0)";
				break;
			case boolType:
				oss << p.first << "(false)";
				break;
			case doubleType:
				oss << p.first << "(0.0)";
				break;
			case stringType:
				oss << p.first << "(\"\")";
				break;
			default:
				break;
		}
	}

	void separator() {
		oss << ", ";
	}
};

struct list_arguments
{
	std::ostream& oss;
	const typeList& tList;

	list_arguments(std::ostream& oss, const typeList& tList) :
		oss(oss), tList(tList)
	{}

	void operator() (const std::pair<std::string, symbol>& p)
	{
		type t = tList.get(p.second.t);
		oss << t.type_name();
		if (t.t == stringType || t.t == structType || t.t == opaqueType)
			oss << " const & ";
		else
			oss << " ";
		oss << p.first;
	}

	void separator() {
		oss << ", ";
	}
};

struct base_construct
{
	std::ostream& oss;

	base_construct(std::ostream& oss) : oss(oss) {}

	void operator() (const std::pair<std::string, symbol>& p)
	{
		oss << p.first << "(" << p.first << ")";
	}

	void separator() {
		oss << ", ";
	}
};

struct dump_types_vis : public boost::static_visitor<void>
{
	std::ostream & oss;
	const typeList& tList;
	std::string name;

	dump_types_vis(std::ostream& oss_, const typeList& tList_, const std::string& name_):
		oss(oss_), tList(tList_), name(name_) {};

	void operator() (const Nothing& n) const { (void)n; }

	void operator() (const typeId& tId) const
	{
		type t = tList.get(tId);
		oss << "\ttypedef " << t.type_name() << " " << scope::get_identifier(name) << ";";
		oss << "\n" << std::endl;
	}

	void operator() (const boost::shared_ptr<symbolList>& l) const
	{
		oss << "\tstruct " << scope::get_identifier(name) << " {" << std::endl;
		oss << "\t\tprivate:" << std::endl;
		oss << "\t\t\tfriend class boost::serialization::access;" << std::endl;
		oss << "\t\t\ttemplate<class Archive>" << std::endl;
		oss << "\t\t\tvoid serialize(Archive & ar, const unsigned int version)" << std::endl;
		oss << "\t\t\t{" << std::endl;
		oss << "\t\t\t\t(void) version;" << std::endl;
		oss << "\t\t\t\tar";
		std::for_each(l->begin(), l->end(), dump_serialize_struct(oss));
		oss << ";" << std::endl;
		oss << "\t\t\t}"  << std::endl;
		oss << "\t\tpublic:" << std::endl;
		std::for_each(l->begin(), l->end(), dump_struct(oss, tList));

		// generate ctor
		oss << "\n\t\t\t" << scope::get_identifier(name) << "() : ";
		hyper::compiler::list_of(l->begin(), l->end(), default_construct(oss, tList));
		oss << "{} \n\n\t\t\t" << scope::get_identifier(name) << "(";
		hyper::compiler::list_of(l->begin(), l->end(), list_arguments(oss, tList));
		oss << ") : \n\t\t\t\t";
		hyper::compiler::list_of(l->begin(), l->end(), base_construct(oss));
		oss << "{}\n";
		oss << "\t};\n" << std::endl;
	}
};

void
type::output(std::ostream& oss, const typeList& tList) const
{
	boost::apply_visitor(dump_types_vis(oss, tList, name), internal);
}

/*
 * XXX Base type must be handled in only one place 
 */
std::string type::type_name() const
{ 
	if (name == "string")
		return "std::string";
	else if (name == "void")
		return "boost::mpl::void_";
	else if (name == "identifier")
		return "hyper::model::identifier";
	else
		return name;
}

struct user_defined_visitor : public boost::static_visitor<bool>
{
	template <typename T>
	bool operator() (const T& ) const { return true; }

	bool operator() (Nothing ) const { return false; }
};

bool type::is_user_defined() const
{
	return boost::apply_visitor(user_defined_visitor(), internal);
}
	
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
		(void) decl;
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

		std::pair < bool, typeId> ref = tList.getId(decl.oldname);
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

	typeList::add_result operator() (const opaque_decl& decl)
	{
		typeList::native_decl_error error;
		error.err = typeList::notTested;

		typeList::add_result r = tList.add(decl.name, opaqueType);
		if (r.get<0>() == false) {
			error.err = typeList::typeAlreadyExist;
			r.get<2>() = error;
			return r;
		}

		type& t = tList.get(r.get<1>());
		t.internal = Nothing();

		error.err = typeList::noError;
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


std::pair <bool, typeId> typeList::getId(const std::string &name) const
{
	typeMap::const_iterator it = mapsId.find(name);
	if (it == mapsId.end())
		return std::make_pair(false, 0);
	return std::make_pair(true, it->second);
}

type typeList::get(typeId t) const
{
	assert(t < types.size());
	return types[t];
}

type& typeList::get(typeId t)
{
	assert(t < types.size());
	return types[t];
}


