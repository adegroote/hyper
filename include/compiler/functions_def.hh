#ifndef _FUNCTIONS_DEF_HH_
#define _FUNCTIONS_DEF_HH_

#include <cassert>
#include <string>
#include <sstream>
#include <stdexcept>

#include <compiler/types.hh>

#include <boost/array.hpp>

namespace hyper {
	namespace compiler {

		class FunctionDefException :  public std::runtime_error
		{
			protected:
				std::string functionName;

			public:
				FunctionDefException(const std::string& name) :
					std::runtime_error("FunctionDefException"),
					functionName(name) {};

				virtual ~FunctionDefException () throw() {};
				virtual const char* what () throw()  = 0;

		};

		class TypeFunctionDefException : public FunctionDefException
		{
			private:
				std::string typeName;

			public:
				TypeFunctionDefException(const std::string &name, const std::string &type):
					FunctionDefException(name), typeName(type) {};
				virtual ~TypeFunctionDefException () throw() {};
				const char* what() throw () 
				{
					std::ostringstream oss;
					oss << "Undefined type " << typeName << "used when defining";
					oss << functionName << std::endl;
					return oss.str().c_str();
				};
		};

		template <size_t N>
		class functionDefImpl 
		{
			private:
				typeList::typeId returnType_;
				boost::array < typeList::typeId, N> argsType_;
				std::string name_;
				const typeList & lists;

			public:
				functionDefImpl(const std::string &name, const std::string& returnName,
								const boost::array < std::string, N> & argsName, 
								const typeList & lists_) : name_(name), lists(lists_)
				{
					std::pair < bool, typeList::typeId > p;
					p = lists.getId(returnName);
					if (p.first == false) 
						throw TypeFunctionDefException(name, returnName);
					else
						returnType_ = p.second;

					for (size_t i = 0; i < N; i++)
					{
						p = lists.getId(argsName[i]);
						if (p.first == false)
							throw TypeFunctionDefException(name, argsName[i]);
						else
							argsType_[i] = p.second;
					}
				}

				size_t arity() const {
					return N;
				};

				type returnType() const {
					return lists.get(returnType_);
				};

				type argsType(size_t i) const {
					assert( i < N);
					return lists.get(argsType_[i]);
				};

				const std::string& name() const {
					return name_;
				}
		};
	};
};

#endif /* _FUNCTIONS_DEF_HH_ */
