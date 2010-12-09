
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
							   network::inform_death_agent> input_msg;
	typedef boost::mpl::vector<network::variable_value,
							   network::request_constraint_ack,
							   network::request_constraint_answer,
							   boost::mpl::void_> output_msg;

	typedef boost::make_variant_over<input_msg>::type input_variant;
	typedef boost::make_variant_over<output_msg>::type output_variant;

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
			a.logger(DEBUG) << " Value updated " << std::endl;
			if (e) {
				network::variable_value* ans(new network::variable_value());
				ans->id = m.id;
				ans->src = m.src;
				ans->var_name = m.var_name;
				ans->success = false;
				return a.client_db[m.src].async_write(*ans, 
						boost::bind(&handle_write_value,
							boost::asio::placeholders::error,
							ans));
			}

			boost::shared_lock<boost::shared_mutex> lock(a.mtx);
			proxy_vis(m);
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

		output_variant operator() (const network::request_constraint& r) const
		{
			model::logic_constraint ctr;
			ctr.constraint = r.constraint;
			ctr.id = r.id;
			ctr.src = r.src;

			a.logger(INFORMATION) << ctr << " Handling ";
			a.logger(INFORMATION) << r.constraint << std::endl;

			a.logic().async_exec(ctr);

			return boost::mpl::void_();
		}

		output_variant operator() (const network::variable_value& v) const
		{
			a.logger(INFORMATION) << "[" << v.src << "," << v.id << "] Answer " << std::endl;
			return actor_vis(v);
		}

		output_variant operator() (const network::request_constraint_answer& v) const
		{
			a.logger(INFORMATION) << "[" << v.src << "," << v.id << "] Answer " << std::endl;
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

			return boost::mpl::void_();
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

			boost::scoped_ptr<tcp_ability_impl> serv;
			network::ping_process ping;
			model::logic_layer logic;

			ability_impl(ability& a_, int level) :
				a(a_), 
				logger(a.io_s, a.name, "logger", a.name_client, level),
				vis(a_),
				ping(a.io_s, boost::posix_time::milliseconds(100), a.name, "localhost", "4242"),
				logic(a_)
			{}
		};

		ability::ability(const std::string& name_, int level) : 
			name_client(io_s, "localhost", "4242"),
			client_db(*this),
			base_id(0),
			updater(*this),
			name(name_),
			impl(new ability_impl(*this, level))
		{
			std::pair<bool, boost::asio::ip::tcp::endpoint> p;
			p = name_client.register_name(name);
			if (p.first == true) {
				std::cout << "Succesfully get an addr " << p.second << std::endl;
				impl->serv.reset(new ability_impl::tcp_ability_impl(p.second, impl->vis, io_s));
			}
			else
				std::cout << "failed to get an addr" << std::endl;
		};
	
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
			impl->serv->stop();
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
