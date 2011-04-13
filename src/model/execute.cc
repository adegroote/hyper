#include <model/execute.hh>

using namespace hyper::model;

void functions_map::add(const std::string& s, function_execution_base* f)
{
	m[s] = boost::shared_ptr<function_execution_base>(f);
}

function_execution_base* functions_map::get(const std::string& s)
{
	return m[s]->clone();
}
