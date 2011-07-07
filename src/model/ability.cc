#include <model/ability.hh>
#include <model/logic_layer.hh>

#include <network/log.hh>
#include <network/ping.hh>
#include <network/server_tcp_impl.hh>

namespace {
	using namespace hyper;

	void handle_write_value(const boost::system::error_code& ,
			network::variable_value* ans)
	{
		delete ans;
	}

	typedef boost::mpl::vector<network::request_variable_value,
							   network::request_constraint,
							   network::variable_value,
							   network::request_constraint_answer,
							   network::inform_new_agent,
							   network::list_agents,
							   network::inform_death_agent,
							   network::terminate> input_msg;
	typedef boost::mpl::vector<network::variable_value,
							   network::request_constraint_ack,
							   network::request_constraint_answer,
							   boost::mpl::void_> output_msg;

	typedef boost::make_variant_over<input_msg>::type input_variant;
	typedef boost::make_variant_over<output_msg>::type output_variant;

	struct close_dead_agent {
		model::ability &a;

		close_dead_agent(model::ability& a_) : a(a_) {}

		void operator() (const std::string& agent) 
		{
			a.client_db[agent].close();
			a.alive_agents.erase(agent);
		}
	};

	struct ability_visitor : public boost::static_visitor<output_variant>
	{
		model::ability &a;
		network::actor::proxy_visitor<model::ability> proxy_vis;
		network::actor_protocol_visitor<model::ability> actor_vis;

		ability_visitor(model::ability& a_) : 
			a(a_), 
			proxy_vis(a_, a_.serializer),
			actor_vis(a_)
		{}

		void handle_update_value(const boost::system::error_code& e,
				const network::request_variable_value& m) const
		{
			a.logger(DEBUG) << "[" << m.src << ", " << m.id << "]";
			if (e) {
				a.logger(DEBUG) << " Failed to update value " << std::endl;
				network::variable_value* ans(new network::variable_value());
				ans->id = m.id;
				ans->src = m.src;
				ans->var_name = m.var_name;
				ans->success = false;
				return a.client_db[m.src].async_write(*ans, 
						boost::bind(&handle_write_value,
							boost::asio::placeholders::error,
							ans));
			} else {
				a.logger(DEBUG) << " Value succesfully updated " << std::endl;
				boost::shared_lock<boost::shared_mutex> lock(a.mtx);
				proxy_vis(m);
			}
		}

		output_variant operator() (const network::request_variable_value& m) const
		{
			a.logger(INFORMATION) << "[" << m.src << ", " << m.id;
			a.logger(INFORMATION) << "] Request for  the value of " << m.var_name << std::endl;

			a.logger(DEBUG) << "[" << m.src << ", " << m.id << "]";
			a.logger(DEBUG) << " Updating value " << std::endl;

			model::updater::cb_type f = boost::bind(
					&ability_visitor::handle_update_value, this,
					boost::asio::placeholders::error,
					m);
			a.updater.async_update(m.var_name, m.id, m.src, f);
			return boost::mpl::void_();
		}

		void handle_constraint_answer(const boost::system::error_code&, 
				network::request_constraint_answer* ans) const
		{
			a.logger(DEBUG) << " Answer send " << std::endl;
			delete ans;
		}

		void handle_async_exec_completion(const boost::system::error_code& e,
				model::logic_constraint ctr) const
		{
			network::request_constraint_answer* ans = new network::request_constraint_answer();
			ans->id = ctr.id;
			ans->src = ctr.src;
			ans->success = !e; // conversion to bool \o/

			a.logger(DEBUG) << ctr << " Getting answer " << std::endl;
			a.client_db[ctr.src].async_write(*ans,
					boost::bind(&ability_visitor::handle_constraint_answer, this, 
								 boost::asio::placeholders::error,  ans));
		}

		output_variant operator() (const network::request_constraint& r) const
		{
			model::logic_constraint ctr;
			ctr.constraint = r.constraint;
			ctr.id = r.id;
			ctr.src = r.src;
			ctr.repeat = r.repeat;

			a.logger(INFORMATION) << ctr << " Handling ";
			a.logger(INFORMATION) << r.constraint << std::endl;

			a.logic().async_exec(ctr, 
					boost::bind(&ability_visitor::handle_async_exec_completion, this,
								boost::asio::placeholders::error, ctr));

			return boost::mpl::void_();
		}

		output_variant operator() (const network::variable_value& v) const
		{
			a.logger(INFORMATION) << "[" << v.src << ", " << v.id << "] Answer " << std::endl;
			return actor_vis(v);
		}

		output_variant operator() (const network::request_constraint_answer& v) const
		{
			a.logger(INFORMATION) << "[" << v.src << ", " << v.id << "] Answer " << std::endl;
			return actor_vis(v);
		}

		output_variant operator() (const network::inform_death_agent& d) const
		{
			a.logger(INFORMATION) << "Receive information about the death of agent(s) : ";
			std::copy(d.dead_agents.begin(), d.dead_agents.end(), 
					std::ostream_iterator<std::string>(a.logger(INFORMATION), ", "));
			a.logger(INFORMATION) << std::endl;

			std::for_each(d.dead_agents.begin(), d.dead_agents.end(),
					boost::bind(&model::ability::cb_db::cancel, &a.db, _1));
			std::for_each(d.dead_agents.begin(), d.dead_agents.end(),
						  close_dead_agent(a));

			return boost::mpl::void_();
		}

		output_variant operator() (const network::terminate& t) const
		{
			a.logger(INFORMATION) << "Exiting by terminaison request : " << t.reason << std::endl;
			a.stop();
			return boost::mpl::void_();
		}

		output_variant operator() (const network::inform_new_agent& l) const
		{
			a.alive_agents.insert(l.new_agents.begin(), l.new_agents.end());
			return boost::mpl::void_();
		}

		output_variant operator() (const network::list_agents& l) const
		{
			return actor_vis(l);
		}
	};
}

namespace hyper {
	namespace model {
		struct ability_impl {
			typedef network::tcp::server<input_msg, 
										 output_msg, 
										 ability_visitor>
			tcp_ability_impl;

			ability& a;
			ability_visitor vis;

			/* Logger for the system */
			network::logger<network::name_client> logger;

			tcp_ability_impl serv;
			network::ping_process ping;
			model::logic_layer logic;

			ability_impl(ability& a_, int level) :
				a(a_), 
				logger(a.io_s, a.name, "logger", a.name_client, level),
				vis(a_),
				serv(vis, a.io_s),
				ping(a.io_s, boost::posix_time::milliseconds(100), a.name, 
					 a.discover.root_addr(), a.discover.root_port()),
				logic(a_)
			{
				std::cout << "discover " << a.discover.root_addr() << " " << a.discover.root_port() << std::endl;
			}
		};

		ability::ability(const std::string& name_, int level) : 
			discover(),
			name_client(io_s, discover.root_addr(), discover.root_port()),
			client_db(*this),
			base_id(0),
			updater(*this),
			name(name_),
			impl(new ability_impl(*this, level))
		{
			const std::vector<boost::asio::ip::tcp::endpoint>& addrs = impl->serv.local_endpoints();
			bool res = name_client.register_name(name, addrs);
			if (res) {
				std::cout << "Succesfully registring " << name_ << " on " ;
				std::copy(addrs.begin(), addrs.end(), 
						std::ostream_iterator<boost::asio::ip::tcp::endpoint>(std::cout, " "));
				std::cout << std::endl;
			}
			else
				std::cout << "failed to register " << name_ << std::endl;

			boost::shared_ptr<get_list_agents> ptr = boost::make_shared<get_list_agents>();
			client_db["root"].async_request(ptr->first, ptr->second,
					boost::bind(&ability::handle_list_agents, this, boost::asio::placeholders::error,  ptr));

		};

		void ability::handle_list_agents(const boost::system::error_code& e, 
										 boost::shared_ptr<get_list_agents> ptr) 
		{
			if (e) 
				return;

			alive_agents.clear();
			alive_agents.insert(ptr->second.all_agents.begin(), ptr->second.all_agents.end());
		}
	
		void ability::test_run()
		{
			io_s.run();
		}
		
		void ability::run()
		{
			impl->ping.run();
			io_s.run();
		}
	
		void ability::stop()
		{
			impl->ping.stop();
			impl->serv.stop();
		}

		std::ostream& ability::logger(int level)
		{
			return impl->logger(level);
		}

		logic_layer& ability::logic()
		{
			return impl->logic;
		}

		ability::~ability() { delete impl; }
	}
}
