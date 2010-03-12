
#ifndef _FUNCTIONS_CALL_HH_
#define _FUNCTIONS_CALL_HH_

#include <vector>
#include <stdexcept>

#include <compiler/types.hh>
#include <compiler/functions_def.hh>
#include <compiler/symbols.hh>

#include <boost/variant.hpp>

namespace hyper {
	namespace compiler {

		struct functionCallEnvException : public std::runtime_error {
			public:
				functionCallEnvException() :
				   	std::runtime_error("functionCallEnvException") {};
				virtual ~functionCallEnvException() throw() {};
				virtual const char* what() throw() = 0;
		};

		struct functionCallEnvExceptionExpectedFunctionName 
			: public functionCallEnvException {
			private:
				std::string var;

			public:
				functionCallEnvExceptionExpectedFunctionName(const std::string &name) : 
					functionCallEnvException(), var(name) {};

				virtual ~functionCallEnvExceptionExpectedFunctionName() throw() {};

				virtual const char* what() throw() {
					std::ostringstream oss;
					oss << var << " does not seems defined " << std::endl;
					return oss.str().c_str();
				}
		};

		struct functionCallEnvExceptionBadArity : public functionCallEnvException {
			private:
				std::string funcName;
				size_t arityExpected;
				size_t arityReal;

			public:
				functionCallEnvExceptionBadArity(const std::string & name, 
						size_t arity_, size_t arityReal_) :
					functionCallEnvException(), funcName(name), 
					arityExpected(arity_), arityReal(arityReal_) {};

				~functionCallEnvExceptionBadArity() throw() {};

				virtual const char* what() throw() {
					std::ostringstream oss;
					oss << funcName << "expects " << arityExpected << " arguments : ";
					oss << " got " << arityReal << " ! " << std::endl;
					return oss.str().c_str();
				};
		};

		struct functionCallEnvExpectedSymbol : public functionCallEnvException
		{
			private:
				std::string var;

			public:
				functionCallEnvExpectedSymbol(const std::string & name) : 
					functionCallEnvException (), var(name) {};

				~functionCallEnvExpectedSymbol() throw() {};

				virtual const char* what() throw() {
					std::ostringstream oss;
					oss << var << "does not seem to be a valid symbol " << std::endl;
					return oss.str().c_str();
				}
		};

		struct functionCallEnvExceptionInvalidArgsType : public functionCallEnvException
		{
			private:
				std::string functionName;
				std::string symbolName;
				std::string expectedType;
				std::string symbolType;
				std::size_t args;

			public:
				functionCallEnvExceptionInvalidArgsType(
						const std::string & fName_, const std::string & sName_, size_t args_,
						const std::string & fType_, const std::string & sType_) :
					functionCallEnvException(), 
					functionName(fName_), symbolName(sName_), 
					expectedType(fType_), symbolType(sType_), args(args_) {};

				~functionCallEnvExceptionInvalidArgsType() throw() {};

				virtual const char* what() throw() {
					std::ostringstream oss;
					oss << "Expect an argument of type " << expectedType;
					oss << "for argument " << args << "of function " << functionName;
					oss << " : got " << symbolName << " of type " << symbolType << std::endl;
					return oss.str().c_str();
				}
		};

		struct functionCallEnvExceptionInvalidReturnType : public functionCallEnvException
		{
			private:
				std::string funcName;
				std::string funcName2;
				std::string expectedType;
				std::string returnType;
				std::size_t args;

			public:
				functionCallEnvExceptionInvalidReturnType(
						const std::string & fName1_, const std::string & fName2_, size_t args_,
						const std::string & fType_, const std::string & rType_) :
					functionCallEnvException(),
					funcName(fName1_), funcName2(fName2_), 
					expectedType(fType_), returnType(rType_), args(args_) {};

				~functionCallEnvExceptionInvalidReturnType() throw() {};

				virtual const char* what() throw() {
					std::ostringstream oss;
					oss << "Expect an argument of type " << expectedType;
					oss << "for argument " << args << "of function " << funcName;
					oss << " : but " << funcName2 << " returns " << returnType << std::endl;
					return oss.str().c_str();
				}
		};


		class functionCall;
	    typedef boost::variant <boost::recursive_wrapper<functionCall>, symbol > node;
		
		class functionCall {
			private:
				std::vector< node > nodes_;
				functionDef f_;

			public:
				functionCall( const std::vector < node >& nodes, const functionDef f) :
				   	nodes_(nodes), f_(f) {};

				functionDef f() const {
					return f_;
				}

				std::string toString() const;
		};

		
		typedef boost::make_recursive_variant<
			std::string,
			std::vector < boost::recursive_variant_ >
		>::type node_string;

		typedef std::vector < node_string > function_node_string;

		struct functionCallEnv {
				const typeList& tList;
				const symbolList& sList;
				const functionDefList& fList;

				functionCallEnv(const typeList &t_, 
								const symbolList &s_,
								const functionDefList &f_) :
					tList(t_), sList(s_), fList(f_)
				{}

				functionCall make(const function_node_string & s) const;
		};
	};
};

#endif /* _FUNCTIONS_CALL_HH_ */
