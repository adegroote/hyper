#include <compiler/scope.hh>
#include <boost/assign/list_of.hpp>

namespace hyper {
	namespace compiler {
		std::vector<std::string> scope::basicIdentifier =
					boost::assign::list_of("int")("double")("string")("void")("bool");

		bool
		scope::is_scoped_identifier(const std::string& s)  
		{
			std::string::size_type res = s.find("::");
			return (res != std::string::npos);
		}

		bool
		scope::is_basic_identifier(const std::string& s)
		{
			// XXX basicIdentifier is not sorted ?
			std::vector<std::string>::const_iterator begin, end;
			begin = basicIdentifier.begin();
			end = basicIdentifier.end();

			return (std::find(begin, end, s) != end);
		}

		std::string
		scope::add_scope(const std::string& abilityName, const std::string& id) 
		{
			if (is_scoped_identifier(id) || is_basic_identifier(id))
				return id;
			
			return abilityName + "::" + id;
		}
		
		std::pair<std::string, std::string>
		scope::decompose(const std::string& name) 
		{
			std::string::size_type res = name.find("::");
			assert (res != std::string::npos);
			return std::make_pair(
					name.substr(0, res),
					name.substr(res+2, name.size() - (res+2)));
		}
		
		std::string
		scope::get_scope(const std::string& name) 
		{
			if (!is_scoped_identifier(name)) return "";
			std::pair<std::string, std::string> p = decompose(name);
			return p.first;
		}
		
		std::string 
		scope::get_identifier(const std::string& identifier) 
		{
			if (!is_scoped_identifier(identifier))
				return identifier;
			std::pair<std::string, std::string> p = decompose(identifier);
			return p.second;
		}

		std::string 
		scope::get_context_identifier(const std::string& identifier, const std::string& context) 
		{
			if (!is_scoped_identifier(identifier))
				return identifier;
			std::pair<std::string, std::string> p = decompose(identifier);
			if (p.first == context)
				return p.second;
			return identifier;
		}
	}
}
