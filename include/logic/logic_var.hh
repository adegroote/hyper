#ifndef HYPER_LOGIC_LOGIC_VAR_HH_
#define HYPER_LOGIC_LOGIC_VAR_HH_

#include <logic/expression.hh>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace hyper {
	namespace logic {
		struct logic_var {
			typedef std::string identifier_type;

			identifier_type id;
			// must be a Constant<T> but too lazy to create another type
			boost::optional<expression> value; 
			std::vector<expression> unified;

			logic_var() {}
			logic_var(identifier_type id) : id(id) {}
		};

		std::ostream& operator <<(std::ostream& os, const logic_var& l);



		struct adapt_res {
			struct ok {};
			struct conflicting_facts {};

			typedef std::pair<logic_var::identifier_type, logic_var::identifier_type> permutation; 
			typedef std::vector<permutation> permutationSeq;

			typedef boost::variant<
				  ok 
				, conflicting_facts
				, permutationSeq
				, function_call
				>
				type;

			adapt_res() : res(ok()) {}

			template <typename Expr>
			adapt_res(Expr const& res) : res(res) {}

			type res;
		};

		class logic_var_db {
			public:
				typedef boost::bimaps::bimap<
					boost::bimaps::multiset_of<logic_var::identifier_type>,
					boost::bimaps::set_of<expression>
				> bm_type;
				typedef bm_type::value_type value_type;

				typedef std::map<logic_var::identifier_type, logic_var> map_type;

			private:
				bm_type bm;
				map_type m_logic_var;

			public:
				/* Rewrite input fact, replacing symbol by an associated logic_var*/
				adapt_res adapt(const function_call& f);
				const logic_var& get(const logic_var::identifier_type& id) const;

				friend std::ostream& operator<<(std::ostream& os, const logic_var_db& db);
		};
	}
}

#endif /* HYPER_LOGIC_LOGIC_VAR_HH_ */
