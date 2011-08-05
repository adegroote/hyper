#ifndef HYPER_NETWORK_MSG_CONSTRAINT_HH_
#define HYPER_NETWORK_MSG_CONSTRAINT_HH_

#include <string>
#include <vector>
#include <utility>

#include <network/types.hh>


namespace hyper {
	namespace network {
		struct request_constraint
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			typedef std::pair<std::string, std::string> unification_pair;
			typedef std::vector<unification_pair> unification_list;
			mutable identifier id;
			mutable std::string src;
			std::string constraint;
			unification_list  unify_list;
			bool repeat;
		};

		struct request_constraint_ack
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			mutable identifier id;
			mutable std::string src;
			bool acked;
		};

		struct request_constraint_answer
		{
			enum state_ { SUCCESS, FAILURE, INTERRUPTED };

			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			mutable identifier id;
			mutable std::string src;
			state_ state;
		};

		struct abort 
		{
			template <class Archive>
			void serialize(Archive& ar, const unsigned int version);

			std::string src;
			identifier id;

			abort() {};
			abort(const std::string& src, identifier id) : src(src), id(id) {}
		};
	}
}

#endif /* HYPER_NETWORK_MSG_CONSTRAINT_HH_ */
