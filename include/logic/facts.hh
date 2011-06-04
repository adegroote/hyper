#ifndef _LOGIC_FACTS_HH_
#define _LOGIC_FACTS_HH_

#include <logic/expression.hh>
#include <logic/logic_var.hh>

#include <boost/logic/tribool.hpp>

#include <iostream>
#include <set>
#include <vector>

namespace hyper {
	namespace logic {
		struct handle_adapt_res;
		class facts {
			public:
				typedef std::set<function_call> expressionS;
				typedef std::vector<expressionS> factsV;
				typedef expressionS::const_iterator const_iterator;

				/* A list of sub-expression which appears for each category */
				typedef std::set<expression> sub_expressionS;
				typedef std::vector<sub_expressionS> sub_expressionV;
				typedef sub_expressionS::const_iterator sub_const_iterator;

				/* 
				 * Let handle_adapt_res visitor access to add_new_facts, and
				 * apply_permutations, but let them private as they are no part
				 * of the exported interface of the object, but an impl detail
				 */
				friend struct handle_adapt_res;
			private:
				const funcDefList& funcs;
				mutable factsV list;
				mutable sub_expressionV sub_list;
				logic_var_db db;

				size_t size__;

				bool add_new_facts(const function_call& f);
				bool apply_permutations(const adapt_res::permutationSeq& seq);

				template <typename T>
				void resize(functionId id, T& list_) const 
				{
					if (id >= list.size()) {
						assert(id < funcs.size());
						list_.resize(funcs.size());
					}
				}

			public:	
				facts(const funcDefList& funcs_): funcs(funcs_), size__(0), db(funcs) {}

				bool add(const std::string& s);
				bool add(const function_call& f);

				boost::logic::tribool matches(const function_call & e);

				function_call generate(const std::string& s);

				const_iterator begin(functionId id) const { 
					resize(id, list);
					return list[id].begin();
				}

				const_iterator end(functionId id) const { 
					resize(id, list);
					return list[id].end();
				}

				sub_const_iterator sub_begin(functionId id) const {
					resize(id, sub_list);
					return sub_list[id].begin();
				}

				sub_const_iterator sub_end(functionId id) const {
					resize(id, sub_list);
					return sub_list[id].end();
				}

				size_t size(functionId id) const {
					if (id >= list.size())
						list.resize(funcs.size());
					return list[id].size();
				}

				size_t size() const { return size__; }

				size_t max_id() const { return list.size(); }

				friend std::ostream& operator << (std::ostream& os, const facts&);
		};

		std::ostream& operator << (std::ostream& os, const facts&);
	}
}

#endif /* _LOGIC_FACTS_HH_ */
