
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
