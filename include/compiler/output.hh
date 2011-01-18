
#ifndef _OUTPUT_COMPILER_HH_
#define _OUTPUT_COMPILER_HH_

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

namespace hyper {
	namespace compiler {
		struct guards
		{
			std::ostream &oss;
			std::string guard;
		
			guards(std::ostream& oss_, const std::string& abilityName, const std::string& end):
				oss(oss_), guard(abilityName)
			{
				std::transform(guard.begin(), guard.end(), guard.begin(), toupper);
				guard = "_" + guard + end;
		
				oss << "#ifndef " << guard << std::endl;
				oss << "#define " << guard << std::endl;
				oss << std::endl;
			}
		
			~guards()
			{
				oss << std::endl;
				oss << "#endif /* " << guard << " */" << std::endl;
			}
		};
		
		struct namespaces
		{
			std::ostream& oss;
			std::string name;
		
			namespaces(std::ostream& oss_, const std::string& name_) : oss(oss_), name(name_)
			{
				oss << "namespace hyper {" << std::endl;
				oss << "\tnamespace " << name << " {" << std::endl << std::endl;
			}
		
			~namespaces()
			{
				oss << std::endl << "\t}\n}" << std::endl;
			}
		};

		struct anonymous_namespaces
		{
			std::ostream& oss;
		
			anonymous_namespaces(std::ostream& oss_) : oss(oss_)
			{
				oss << "namespace {" << std::endl;
			}
		
			~anonymous_namespaces()
			{
				oss << "\n}" << std::endl;
			}
		};


		struct dump_depends
		{
			std::ostream & oss;
			std::string depend_kind;
			char start;
			
			dump_depends(std::ostream& oss_, const std::string& kind) : 
				oss(oss_), depend_kind(kind), start('#') {};
		
			dump_depends(std::ostream& oss_, const std::string& kind, char start) : 
				oss(oss_), depend_kind(kind), start(start) {};

			void operator() (const std::string& s) const
			{
				if (s == "")
					return;
		
				oss << start << "include <" << s << "/" << depend_kind << ">" << std::endl;
			}
		};

		inline
		std::string quoted_string(const std::string& s)
		{
			return "\"" + s + "\"";
		}

		inline
		std::string times(size_t nb, const std::string& s)
		{
			std::ostringstream oss;
			for (size_t i = 0; i < nb; i++)
				oss << s;
			return oss.str();
		}
	}
}

#endif
