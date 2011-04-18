#ifndef HYPER_UTILS_ALGORITHM_HH_
#define HYPER_UTILS_ALGORITHM_HH_

namespace hyper {
	namespace utils {

		/* Returns true if any value between begin and end matches predicate
		 * Pred */
		template <typename InputIterator, typename Predicate>
		bool any(InputIterator begin, InputIterator end, Predicate Pred)
		{
			for (; begin != end; ++begin) 
				if (Pred(*begin))
						return true;

			return false;
		}

		/* Returns true if all values between begin and end match predicate
		 * Pred */
		template <typename InputIterator, typename Predicate>
		bool all(InputIterator begin, InputIterator end, Predicate Pred)
		{
			for (; begin != end; ++begin)
				if (!Pred(*begin))
					return false;

			return true;
		}
	}
}

#endif /* HYPER_UTILS_ALGORITHM_HH_ */
