#ifndef HYPER_LOGIC_BACKWARD_CHAINING_HH_
#define HYPER_LOGIC_BACKWARD_CHAINING_HH_

#include <logic/engine.hh>
#include <boost/spirit/home/phoenix.hpp> 

namespace hyper {
	namespace logic {
		namespace details {
			typedef size_t node_id;
			typedef size_t hypothesis_id;
			struct tree;

			enum proof_state {
				proven_true,
				proven_false,
				not_proven_not_explored,
				not_proven_exploring,
				not_proven_explored
			};

			struct hypothesis {
				function_call f;
				proof_state state;
				std::vector<node_id> nodes;

				hypothesis() {}
				hypothesis(function_call f) : 
					f(f), state(not_proven_not_explored) 
				{}
			};

			inline
			bool operator< (const hypothesis& h1, const hypothesis& h2)
			{
				return h1.f < h2.f;
			}

			struct node {
				proof_state state;

				node() : state(not_proven_not_explored) {}

				typedef std::vector<hypothesis_id> hypothesisV;
				typedef hypothesisV::const_iterator const_iterator;

				hypothesisV hypos;

				const_iterator cbegin() const { return hypos.begin(); }
				const_iterator cend() const { return hypos.end(); }
			};

			struct generate_node;
			struct explore_node; 

			class proof_tree {
				private:
					// node_id -> node
					typedef std::map<node_id, node> bm_node_type;
					bm_node_type bm_node;

					// hyp_id <-> hyp
					typedef boost::bimaps::bimap<
						boost::bimaps::set_of<hypothesis_id>,
						boost::bimaps::set_of<hypothesis>
							> bm_hyp_type;

					bm_hyp_type bm_hyp;

					node_id node_id_generator;
					hypothesis_id hyp_id_generator;

					facts_ctx& ctx;
					const rules& rs;

					void update_hypothesis(hypothesis_id id, const hypothesis& h);

					node_id insert_node(const node& n);
					hypothesis_id add_hypothesis_to_node(node& n, const function_call& f);

					template <typename InputIterator, typename OutputIterator>
					OutputIterator add_hypothesis_to_node(node &n,
							InputIterator begin, InputIterator end,
							OutputIterator out)
					{
						namespace phx = boost::phoenix;
						node_id (proof_tree::*f)(node&, const function_call&) = &proof_tree::add_hypothesis_to_node;
						return std::transform(begin, end, out, 
								phx::bind(f, *this, phx::ref(n), phx::arg_names::arg1));
					}

					void compute_node_state(node& n);

					node& get_node(node_id);
					void try_solve_node(node & n);
					void explore_hypothesis(hypothesis_id);


					friend struct generate_node;
					friend struct explore_node;

				public:
					proof_tree(facts_ctx& ctx, const rules& rs) : 
						node_id_generator(0), hyp_id_generator(0), ctx(ctx), rs(rs) 
					{}

					boost::logic::tribool compute(const function_call& f);

					void fill_hypothesis(const hypothesis&, std::vector<function_call>& hyps) const;
					void fill_hypothesis(const function_call& f, std::vector<function_call>& hyps) const;

					const node& get_node(node_id) const;
					const hypothesis& get_hypothesis(hypothesis_id) const;

					/* to debug only */
					void print() const;
					void print_node(const node& n, int index) const;
					void print_node(node_id id, int index) const;
					void print_hyp(hypothesis_id, int index) const;

			};

		}

		struct backward_chaining {
			const rules& rs;
			facts_ctx& ctx;

			backward_chaining(const rules& rs, facts_ctx& ctx):
				rs(rs), ctx(ctx)
			{}

			/* Check if f is directly inferable from the facts and the rules */
			bool infer(const function_call& f);

			/* Check if f is inferable from the facts and the rules. If it is
			 * not the case, hyps contains a list of hypothesis. If ANY of this
			 * hypothesis is true (but it is outside the scope of the current
			 * facts / rules), then f can be proved.
			 *
			 * It is quite a simplistic interface, as in the proof tree, we can
			 * have some proof under n hypothesis
			 */
			bool infer(const function_call& f, std::vector<function_call>& hyps);
		};
	}
}

#endif /* HYPER_LOGIC_BACKWARD_CHAINING_HH_ */
