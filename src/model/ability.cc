#include <model/ability.hh>

namespace hyper {
	namespace model {
		namespace details {
				output_variant ability_visitor::operator() 
					(const network::request_variable_value& m) const
				{
					boost::shared_lock<boost::shared_mutex> lock(a.mtx);
					a.logger(INFORMATION) << "[" << m.src << ", " << m.id;
					a.logger(INFORMATION) << "] Request for  the value of " << m.var_name << std::endl;
					return a.proxy_vis(m);
				}

				output_variant ability_visitor::operator()
					(const network::request_constraint& r) const
				{
					size_t current_id = constraint_id++;

					a.logger(INFORMATION) << "Handling a request for enforcing constraint ";
					a.logger(INFORMATION) << r.constraint << std::endl;

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
					a.logger(INFORMATION) << "[" << v.src << "," << v.id << "] Answer " << std::endl;
					return a.actor_vis(v);
				}

				output_variant ability_visitor::operator() 
					(const network::request_constraint_answer& v) const
				{
					a.logger(INFORMATION) << "[" << v.src << "," << v.id << "] Answer " << std::endl;
					return a.actor_vis(v);
				}
		}
	}
}
