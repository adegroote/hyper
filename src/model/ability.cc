#include <model/ability.hh>

namespace hyper {
	namespace model {
		namespace details {
				output_variant ability_visitor::operator() 
					(const network::request_variable_value& m) const
				{
					boost::shared_lock<boost::shared_mutex> lock(a.mtx);
					a.logger(3) << "Handling a request for  the value of " << m.var_name << std::endl;
					return a.proxy_vis(m);
				}

				output_variant ability_visitor::operator()
					(const network::request_constraint& r) const
				{
					size_t current_id = constraint_id++;

					a.logger(3) << "Handling a request for enforcing constraint ";
					a.logger(3) << r.constraint << std::endl;

					logic_constraint ctr;
					ctr.constraint = r.constraint;
					ctr.id = current_id;

					a.logic.async_exec(ctr);

					network::request_constraint_ack ack;
					ack.acked = true;
					ack.id = current_id;
					return ack;
				}

				output_variant ability_visitor::operator() (const network::variable_value& v) const
				{
					return a.actor_vis(v);
				}

				output_variant ability_visitor::operator() 
					(const network::request_constraint_answer& v) const
				{
					return a.actor_vis(v);
				}
		}
	}
}
