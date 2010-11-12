#include <algorithm>
#include <numeric>
#include <model/execute.hh>

using namespace hyper::model;

struct Delete 
{ 
	template <class T> void operator ()(T*& p) const 
	{ 
		delete p;
		p = NULL;
	} 
}; 

/* 
 * We don't need to take care about v_proxy, because a proxy can't only be
 * called by a call of a func instance, so if all func instance are terminated,
 * there is no anymore instance of proxy running
 */
bool
execution_context::is_finished() const
{
	bool res = true;
	std::vector<function_execution_base*>::const_iterator it;
	for (it = v_func.begin(); it != v_func.end(); ++it)
		res &= (*it)->is_finished();
	return res;
}

execution_context::~execution_context()
{
	assert(is_finished());
	std::for_each(v_proxy.begin(), v_proxy.end(), Delete());
	std::for_each(v_func.begin(), v_func.end(), Delete());
}

