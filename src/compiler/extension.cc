#include <compiler/extension.hh>
#include <compiler/expression_ast.hh>
#include <compiler/universe.hh>
#include <compiler/scope.hh>

namespace {
	using namespace hyper::compiler;
	struct is_tagged_func_call_helper : public boost::static_visitor< std::pair<bool, std::string> >
	{
		std::string name;
		const universe& u;
		
		is_tagged_func_call_helper(const std::string name, const universe& u) : 
			name(name), u(u) {}

		template <typename T>
		std::pair<bool, std::string> operator() (const T&) const
		{
			return std::make_pair(false, "");
		}

		std::pair<bool, std::string> operator() (const function_call &f) const
		{
			// add scope to do the search
			std::string func_name = scope::add_scope(name, f.fName);
			std::pair<bool, functionDef> p = u.get_functionDef(func_name);
			if (p.first == false) {
				std::cerr << "Unknow function " << f.fName << std::endl;
				return std::make_pair(false, "");
			}

			const boost::optional<std::string>& tag = p.second.tag();
			if (tag)
				return std::make_pair(true, *tag);

			return std::make_pair(false, "");
		}
	};
}

namespace hyper {
	namespace compiler {
		std::pair<bool, std::string> 
		is_tagged_func_call(const expression_ast& e, const std::string& name, const universe &u)
		{
			return boost::apply_visitor(is_tagged_func_call_helper(name, u), e.expr);
		}
	}
}
