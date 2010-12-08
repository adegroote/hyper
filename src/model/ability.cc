#include <model/ability.hh>

namespace {
	using namespace hyper;

	void handle_write_value(const boost::system::error_code& ,
			network::variable_value* ans)
	{
		delete ans;
	}

	void handle_update_value(const boost::system::error_code& e,
			model::ability &a,
			const network::request_variable_value& m)
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
		a.proxy_vis(m);
	}
}

namespace hyper {
	namespace model {
		namespace details {


				output_variant ability_visitor::operator() 
					(const network::request_variable_value& m) const
				{
					a.logger(INFORMATION) << "[" << m.src << ", " << m.id;
					a.logger(INFORMATION) << "] Request for  the value of " << m.var_name << std::endl;

					a.logger(DEBUG) << "[" << m.src << ", " << m.id << "]";
					a.logger(DEBUG) << " Updating value " << std::endl;

					model::updater::cb_type f = boost::bind(&handle_update_value,
												boost::asio::placeholders::error,
												boost::ref(a), m);
					a.updater.async_update(m.var_name, f);
					return boost::mpl::void_();
				}

				output_variant ability_visitor::operator()
					(const network::request_constraint& r) const
				{

					logic_constraint ctr;
					ctr.constraint = r.constraint;
					ctr.id = r.id;
					ctr.src = r.src;

					a.logger(INFORMATION) << ctr << " Handling ";
					a.logger(INFORMATION) << r.constraint << std::endl;

					a.logic.async_exec(ctr);

					return boost::mpl::void_();
				}

				output_variant ability_visitor::operator() (const network::variable_value& v) const
				{
					a.logger(INFORMATION) << "[" << v.src << "," << v.id << "] Answer " << std::endl;
					return a.actor_vis(v);
				}

				output_variant ability_visitor::operator() 
					(const network::request_constraint_answer& v) const
				{
					a.logger(INFORMATION) << "[" << v.src << "," << v.id << "] Answer " << std::endl;
					return a.actor_vis(v);
				}

				output_variant ability_visitor::operator() 
					(const network::inform_death_agent& d) const
				{
					a.logger(INFORMATION) << "Receive information about the death of agent(s) : ";
					std::copy(d.dead_agents.begin(), d.dead_agents.end(), 
							  std::ostream_iterator<std::string>(a.logger(INFORMATION), ", "));
					a.logger(INFORMATION) << std::endl;

					std::for_each(d.dead_agents.begin(), d.dead_agents.end(),
								  boost::bind(&ability::cb_db::cancel, &a.db, _1));

					return boost::mpl::void_();
				}
		}
	}
}
