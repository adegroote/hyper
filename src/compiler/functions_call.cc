
#include <compiler/functions_call.hh>

using namespace hyper::compiler;

struct convertToNode : public boost::static_visitor<node>  
{
	const std::string funcName;
	typeList::typeId id;
	size_t arg_number;
	const functionCallEnv *env;

	convertToNode(const functionCallEnv* env_,
				  const std::string &funcName_,
				  typeList::typeId id_,
				  size_t arg_number_) : 
		boost::static_visitor<node> (),
		funcName(funcName_), id(id_), arg_number(arg_number_), env(env_)
	{};

	node operator() (const std::string & sym) const 
	{
		std::pair < bool, symbol > p;
		p = env->sList.get(sym);
		if (p.first == false) 
			throw functionCallEnvExpectedSymbol(sym);

		if (p.second.t != id) 
			throw functionCallEnvExceptionInvalidArgsType(funcName, sym, arg_number,
					env->tList.get(id).name, env->tList.get(p.second.t).name);
		return p.second;
	};

	node operator() (const function_node_string & v) {
		functionCall res = env->make(v);
		if (res.f().returnType() != id) 
			throw functionCallEnvExceptionInvalidReturnType(
					funcName, res.f().name(),
					arg_number, 
					env->tList.get(id).name,
				   	env->tList.get(res.f().returnType()).name);
		return res;
	}
};

functionCall
functionCallEnv::make(const function_node_string & v) const
{
	assert(v.empty() == false);
	const std::string* s = boost::get<std::string> (&v[0]);

	assert(s != NULL);

	std::pair < bool, functionDef > p;
	p = fList.get(*s);
	if (p.first == false) 
		throw functionCallEnvExceptionExpectedFunctionName(*s);
	functionDef fun = p.second;

	if (v.size()-1 != fun.arity()) 
		throw functionCallEnvExceptionBadArity(fun.name(), fun.arity(), v.size()- 1);

	std::vector < node > functionArgs;
	for (size_t i = 1; i < v.size(); ++i)
	{
		convertToNode convertVisitor(this, fun.name(), fun.argsType(i-1), i - 1);
		functionArgs.push_back(boost::apply_visitor ( convertVisitor, v[i] ));
	}

	return functionCall(functionArgs, fun);
}

struct printCallNode : public boost::static_visitor<std::string>  
{
	std::string operator() (const symbol & s) const 
	{
		return s.name;
	}

	std::string operator() (const functionCall & fc) const
	{
		return fc.toString();
	}
};

std::string  functionCall::toString() const
{
	std::ostringstream oss;
	oss << f().name() << "(";
	for (size_t i = 0; i < nodes_.size(); i++) {
		oss << boost::apply_visitor(printCallNode(), nodes_[i]);
		if ( i != nodes_.size() - 1)
			oss << ", ";
	}
	oss << ")";
	return oss.str();
}
