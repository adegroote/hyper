#include <model/update.hh>
#include <model/ability.hh>
#include <model/logic_layer.hh>


namespace {
	void pass(hyper::network::identifier id, const std::string& src, 
			  hyper::model::updater::cb_type cb)
	{
		(void) id; (void) src;
		return cb(boost::system::error_code());
	}

	void handle_call_task_terminaison(hyper::network::request_constraint_answer::state_ s,
								 hyper::model::updater::cb_type cb)
	{
		switch (s) {
			case hyper::network::request_constraint_answer::RUNNING:
			case hyper::network::request_constraint_answer::PAUSED:
			case hyper::network::request_constraint_answer::TEMP_FAILURE:
				break;
			case hyper::network::request_constraint_answer::SUCCESS:
				cb(boost::system::error_code());
				break;
			case hyper::network::request_constraint_answer::FAILURE:
				cb(make_error_code(boost::system::errc::invalid_argument)); // XXX
				break;
			case hyper::network::request_constraint_answer::INTERRUPTED:
				cb(make_error_code(boost::system::errc::interrupted));
				break;
		}
	}

	void call_task(hyper::model::ability&a, const std::string& var,
				   hyper::network::identifier id, const std::string& src,
				   hyper::model::updater::cb_type cb)
	{
		a.logic().async_exec(var, id, src, 
				boost::bind(&handle_call_task_terminaison, _1, cb));
	}

	void handle_update_var(const boost::system::error_code& e,
						   hyper::model::local_vars& vars, size_t i, 
						   hyper::model::updater::cb_type cb)
	{
		vars.vars[i].is_updated = true;
		if (vars.is_terminated())
			cb(e);
	}
}

namespace hyper {
	namespace model {

		updater::updater(ability& a_) : a(a_) {}

		bool updater::remove(const std::string& var) 
		{
			map_type::iterator it = map.find(var);
			if (it == map.end())
				return false;
			else {
				map.erase(it);
				return true;
			}
		}

		bool updater::add(const std::string& var) 
		{
			std::pair<map_type::iterator, bool> p;
			p = map.insert(std::make_pair(var, pass));
			return p.second;
		}

		bool updater::add(const std::string& var, const std::string& task)
		{
			fun_type f = boost::bind(call_task, boost::ref(a),
											    task,
											    _1, _2, _3);
			std::pair<map_type::iterator, bool> p;
			p = map.insert(std::make_pair(var, f));
			return p.second;
		}

		void updater::async_update(const std::string& var, network::identifier id,
								   const std::string& src, cb_type cb)
		{
			map_type::const_iterator it = map.find(var);
			if (it == map.end())
				return cb(boost::asio::error::invalid_argument);

			return it->second(id, src, cb);
		}

		void updater::async_update(local_vars& vars, network::identifier id,
								   const std::string& src, cb_type cb)
		{
			vars.reset();
			for (size_t i=0; i < vars.vars.size(); ++i)
				async_update(vars.vars[i].var, id, src, 
							 boost::bind(handle_update_var,
										 boost::asio::placeholders::error,
										 boost::ref(vars), i, cb));
		}
	}
}
