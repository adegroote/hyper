#ifndef HYPER_MODEL_MODULE_ABILITY_TEST_HH
#define HYPER_MODEL_MODULE_ABILITY_TEST_HH

#include <map>

#include <compiler/universe.hh>
#include <compiler/parser.hh>
#include <compiler/symbols.hh>
#include <compiler/symbols_parser.hh>
#include <model/ability.hh>
#include <model/future.hh>
#include <model/proxy.hh>
#include <network/log_level.hh>
#include <network/json_archive.hh>
#include <network/msg_constraint.hh>

#include <boost/function/function0.hpp>

namespace hyper {
	namespace model {
		struct ability_test : public ability {
			std::string target;
			remote_proxy proxy;
			typedef std::map<std::string, boost::function<void ()> > getter_map;
			getter_map gmap;
			typedef std::map<std::string, 
							 boost::function<bool (const std::string&, const std::string&)> 
							> factory_map;
			factory_map fmap;
			hyper::compiler::universe u;
			hyper::compiler::parser p;
			hyper::compiler::symbolList locals;
			
			ability_test(const std::string& name_);

			private:
			template <typename T>
			void handle_get(const boost::system::error_code&e,
							remote_value<T>& value)
			{
				if (e) {
					std::cerr << "Failed to get the value of " << value.msg.var_name << std::endl;
				} else {
					std::ostringstream oss;
					hyper::network::json_oarchive oa(oss);
					oa << value.value;

					std::cerr << "Get \"" << value.msg.var_name << "\": "<< oss.str() << std::endl;
				}
				this->stop();
			}

			void handle_send_constraint(const boost::system::error_code& e,
										network::identifier id,
										future_value<bool>, 
									    network::request_constraint2* msg,
										network::request_constraint_answer* ans);

			future_value<bool> send_constraint(const compiler::recipe_expression& expr, bool repeat);

			template <typename T>
			void get(remote_value<T>& value)
			{
				void (ability_test::*f) (const boost::system::error_code&,
										 remote_value<T>&) =
					&ability_test::template handle_get<T>;

				proxy.async_get(value,
						boost::bind(f, this,
									boost::asio::placeholders::error,
									boost::ref(value)));
			}

			/* 
			 * Yes, really pass a copy of tname, because the reference on tname
			 * may have dissaperead when it is really called
			 */
			template <typename T>
			bool generate_new_local_variable(std::string tname,
											 const std::string& name, 
											 const std::string& json_value)
			{
				/*
				 * XXX The current code leaks, but it is not so important,
				 * considering the lifetime of ability_test, or the number of
				 * 'local variable'.
				 */
				T* var = new T();
				std::istringstream iss(json_value);
				hyper::network::json_iarchive ia(iss);
				try {
					ia >> *var;
				} catch (const std::runtime_error& e) {
					std::cerr << "Failed to decode properly " << json_value ;
					std::cerr << " in a variable of type " << tname;
					std::cerr << " : " << e.what();
					return false;
				}

				export_local_variable(name, *var);

				hyper::compiler::symbol_decl decl;
				decl.typeName = tname;
				decl.name = name;
				hyper::compiler::symbolList::add_result r = locals.add(decl);
				if (!r.first) {
					std::cerr << locals.get_diagnostic(decl, r) << std::endl;
				}
				return r.first;
			}

			public:
			template <typename T>
			void register_get(const std::string& s, remote_value<T>& t)
			{
				void (ability_test::*f) (remote_value<T>&) = 
					&ability_test::template get<T>;
				gmap[s] = boost::bind(f, this, t);
			}

			template <typename T>
			void register_factory(const std::string& tname)
			{
				bool (ability_test::*f) (std::string,
										 const std::string&,
										 const std::string&) =
					&ability_test::template generate_new_local_variable<T>;

				fmap[tname] = boost::bind(f, this, tname, _1, _2);
			}

			int main(int argc, char** argv);

            static bool correct_usage(int argc, char **argv);
            static int usage(const std::string&);
		};
	}
}

#endif /* HYPER_MODEL_MODULE_ABILITY_TEST_HH */
