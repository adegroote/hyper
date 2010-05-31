
#include <compiler/ability.hh>

using namespace hyper::compiler;

std::pair<bool, symbolACL>
ability::get_symbol(const std::string& name) const
{
	std::pair<bool, symbol> p;
	p = controlable_list.get(name);
	if (p.first == true) 
		return std::make_pair(true, symbolACL(p.second, CONTROLABLE));

	p = readable_list.get(name);
	if (p.first == true) 
		return std::make_pair(true, symbolACL(p.second, READABLE));

	p = private_list.get(name);
	if (p.first == true)
		return std::make_pair(true, symbolACL(p.second, PRIVATE));

	return std::make_pair(false, symbolACL());
}

struct same_name {
	std::string name;

	same_name(const std::string& name_): name(name_) {};

	bool operator() (const task& t) const
	{
		return (t.name == name);
	}
};

bool
ability::add_task(const task& t)
{
	/*
	 * Verify that there is no task with the same name
	 * It is not really efficient, but :
	 *   - we don't care a lot about performance here
	 *   - we probably won't have a lot of task, so O(n) against O(ln n) is
	 *   probably not so important
	 */
	std::vector<task>::const_iterator it;
	it = std::find_if(tasks.begin(), tasks.end(), same_name(t.name));

	if (it != tasks.end())
		return false;

	tasks.push_back(t);
	return true;
}
