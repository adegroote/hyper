#include <model/ability.hh>
#include <model/actor_impl.hh>
#include <model/logic_layer.hh>
#include <network/proxy_visitor.hh>

#include <network/log_level.hh>
#include <network/msg.hh>
#include <network/ping.hh>
#include <network/server_tcp_impl.hh>

namespace hyper { namespace model {
	void handle_constraint_answer(hyper::model::ability &a, 
			const boost::system::error_code&, 
			network::request_constraint_answer* ans) 
	{
		delete ans;
	}

	void update_ctr_status(hyper::model::ability& a, hyper::model::logic_constraint ctr)
	{
		if (ctr.internal)
			return;

		network::request_constraint_answer* ans = new network::request_constraint_answer();
		ans->id = ctr.id;
		ans->src = ctr.src;
		ans->state = ctr.s;

		a.logger(DEBUG) << ctr << " Sending constraint update status " ;
		switch(ans->state) {
			case network::request_constraint_answer::INIT:
				a.logger(DEBUG) << "init";
				break;
			case network::request_constraint_answer::RUNNING:
				a.logger(DEBUG) << "running";
				break;
			case network::request_constraint_answer::PAUSED:
				a.logger(DEBUG) << "paused";
				break;
			case network::request_constraint_answer::TEMP_FAILURE:
				a.logger(DEBUG) << "temporary_failure";
				break;
			case network::request_constraint_answer::SUCCESS:
				a.logger(DEBUG) << "success";
				break;
			case network::request_constraint_answer::FAILURE:
				a.logger(DEBUG) << "failure";
				break;
			case network::request_constraint_answer::INTERRUPTED:
				a.logger(DEBUG) << "interrupted";
				break;
		}
		a.logger(DEBUG)	<< std::endl;

		a.actor->client_db[ctr.src].async_write(*ans,
				boost::bind(&handle_constraint_answer, boost::ref(a), 
					boost::asio::placeholders::error,  ans));
	}
}}

namespace {
	using namespace hyper;

	void handle_write_value(const boost::system::error_code& ,
			network::variable_value* ans)
	{
		delete ans;
	}


	typedef boost::mpl::vector<network::request_variable_value,
							   network::request_constraint,
							   network::request_constraint2,
							   network::variable_value,
							   network::request_constraint_answer,
							   network::inform_new_agent,
							   network::list_agents,
							   network::inform_death_agent,
							   network::terminate,
							   network::abort,
							   network::pause,
							   network::resume> input_msg;
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
			a.actor->client_db[agent].close();
			a.alive_agents.erase(agent);
		}
	};

	struct ability_visitor : public boost::static_visitor<output_variant>
	{
		model::ability &a;
		network::proxy_visitor<model::actor_impl> proxy_vis;
		network::actor_protocol_visitor<model::actor_impl> actor_vis;

		ability_visitor(model::ability& a_) : 
			a(a_), 
			proxy_vis(*a_.actor, a_.serializer),
			actor_vis(*a_.actor)
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
				return a.actor->client_db[m.src].async_write(*ans, 
						boost::bind(&handle_write_value,
							boost::asio::placeholders::error,
							ans));
			} else {
				a.logger(DEBUG) << " Value succesfully updated " << std::endl;
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



		void handle_async_exec_completion(network::request_constraint_answer::state_ s,
				model::logic_constraint ctr) const
		{
			model::logic_constraint ctr2 = ctr;
			ctr2.s = s;
			update_ctr_status(a, ctr2);
		}

		output_variant operator() (const network::request_constraint& r) const
		{
			model::logic_constraint ctr;
			ctr.id = r.id;
			ctr.src = r.src;
			ctr.repeat = r.repeat;
			ctr.s = network::request_constraint_answer::INIT;
			ctr.internal = false;

			a.logger(INFORMATION) << ctr << " Handling ";
			a.logger(INFORMATION) << r.constraint << std::endl;

			a.logic().async_exec(ctr, r.constraint, r.unify_list, 
					boost::bind(&ability_visitor::handle_async_exec_completion, this,
								boost::asio::placeholders::error, ctr));

			return boost::mpl::void_();
		}

		output_variant operator() (const network::request_constraint2& r) const
		{
			model::logic_constraint ctr;
			ctr.id = r.id;
			ctr.src = r.src;
			ctr.repeat = r.repeat;
			ctr.s = network::request_constraint_answer::INIT;
			ctr.internal = false;

			a.logger(INFORMATION) << ctr << " Handling ";
			a.logger(INFORMATION) << r.constraint << std::endl;

			a.logic().async_exec(ctr, r.constraint, r.unify_list,
					boost::bind(&ability_visitor::handle_async_exec_completion, this,
								boost::asio::placeholders::error, ctr));

			return boost::mpl::void_();
		}

		output_variant operator() (const network::variable_value& v) const
		{
			a.logger(INFORMATION) << "[" << v.src << ", " << v.id << "] Final answer " << std::endl;
			return actor_vis(v);
		}

		output_variant operator() (const network::request_constraint_answer& v) const
		{
			switch (v.state) {
			case network::request_constraint_answer::INIT:
			case network::request_constraint_answer::RUNNING:
			case network::request_constraint_answer::PAUSED:
			case network::request_constraint_answer::TEMP_FAILURE:
				a.logger(INFORMATION) << "[" << v.src << ", " << v.id << "] Answer " << std::endl;
				break;
			case network::request_constraint_answer::SUCCESS:
			case network::request_constraint_answer::FAILURE:
			case network::request_constraint_answer::INTERRUPTED:
				a.logger(INFORMATION) << "[" << v.src << ", " << v.id << "] Final answer " << std::endl;
				break;
			}
			return actor_vis(v);
		}

		output_variant operator() (const network::inform_death_agent& d) const
		{
			a.logger(INFORMATION) << "Receive information about the death of agent(s) : ";
			std::copy(d.dead_agents.begin(), d.dead_agents.end(), 
					std::ostream_iterator<std::string>(a.logger(INFORMATION), ", "));
			a.logger(INFORMATION) << std::endl;

			std::for_each(d.dead_agents.begin(), d.dead_agents.end(),
					boost::bind(&model::actor_impl::cb_db::cancel, &a.actor->db, _1));
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

		output_variant operator() (const network::abort& abort) const
		{
			a.logger(INFORMATION) << "[" << abort.src << ", " << abort.id << "] Abort requested " << std::endl;
			a.logic().abort(abort.src, abort.id);
			return boost::mpl::void_();
		}

		output_variant operator() (const network::pause& pause) const
		{
			a.logger(INFORMATION) << "[" << pause.src << ", " << pause.id << "] Pause requested " << std::endl;
			a.logic().pause(pause.src, pause.id);
			return boost::mpl::void_();
		}

		output_variant operator() (const network::resume& resume) const
		{
			a.logger(INFORMATION) << "[" << resume.src << ", " << resume.id << "] Resume requested " << std::endl;
			a.logic().resume(resume.src, resume.id);
			return boost::mpl::void_();
		}
	};
}

namespace hyper {
	namespace model {
		struct get_list_agents {
			network::request_list_agents first;
			network::list_agents second;
		};

		struct ability_impl {
			typedef network::tcp::server<input_msg, 
										 output_msg, 
										 ability_visitor>
			tcp_ability_impl;

			ability& a;
			ability_visitor vis;


			tcp_ability_impl serv;
			network::ping_process ping;
			model::logic_layer logic;

			ability_impl(ability& a_, int level) :
				a(a_), 
				vis(a_),
				serv(vis, a.io_s),
				ping(a.io_s, boost::posix_time::milliseconds(100), a.name, 
					 a.discover.root_addr(), a.discover.root_port()),
				logic(a_)
			{
				std::cout << "discover " << a.discover.root_addr() << " " << a.discover.root_port() << std::endl;
			}
		};
		actor_impl::actor_impl(boost::asio::io_service& io_s, const std::string& name, int level, 
							   const discover_root& discover):
			io_s(io_s), name(name),
			name_client(io_s, discover.root_addr(), discover.root_port()),
			client_db(*this),
			base_id(0),
			logger_(io_s, name, "logger", name_client, level)
		{}

		ability::ability(const std::string& name_, int level) : 
			discover(),
			actor(new actor_impl(io_s, name_, level, discover)),
			updater(*this),
			setter(*this),
			name(name_),
			impl(new ability_impl(*this, level))
		{};

		void ability::start()
		{
			const std::vector<boost::asio::ip::tcp::endpoint>& addrs = impl->serv.local_endpoints();
			bool res = actor->name_client.register_name(name, addrs);
			if (res) {
				std::cout << "Succesfully registring " << name << " on " ;
				std::copy(addrs.begin(), addrs.end(), 
						std::ostream_iterator<boost::asio::ip::tcp::endpoint>(std::cout, " "));
				std::cout << std::endl;
			}
			else
				std::cout << "failed to register " << name << std::endl;

			boost::shared_ptr<get_list_agents> ptr = boost::make_shared<get_list_agents>();
			actor->client_db["root"].async_request(ptr->first, ptr->second,
					boost::bind(&ability::handle_list_agents, this, boost::asio::placeholders::error, _2, ptr));
		}

		void ability::handle_list_agents(const boost::system::error_code& e, 
									     network::identifier id,
										 boost::shared_ptr<get_list_agents> ptr) 
		{
			actor->db.remove(id);

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
			return actor->logger(level);
		}

		logic_layer& ability::logic()
		{
			return impl->logic;
		}

		ability::~ability() { delete impl;  delete actor; }
	}
}
