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

		/*
		 * copy_if copies elements from the range [first, last] to a
		 * range beginning at res, if that elements fullfil the pred
		 * The return value is the end of the resulting range. This operation
		 * is stable, meaning that the relative order of the elements that are
		 * copied is the same as in the range [first, last].
		 *
		 * Predicate must be an adaptable functor
		 */
		template <typename InputIterator, typename OutputIterator, typename Predicate>
		OutputIterator 
		copy_if(InputIterator begin, InputIterator last, OutputIterator res, Predicate pred)
		{
			while (begin != last) {
				if (pred(*begin))
					*res++ = *begin;
				*begin++;
			}

			return res;
		}

		/*
		 * transform_if apply the transformation op on element in the range
		 * [first, last] to a range beginning at res, if that element fullfil pred.
		 * The return value is the end of the resulting range
		 */
		template <typename InputIterator, typename OutputIterator, typename Predicate, typename UnaryOp>
		OutputIterator
		transform_if(InputIterator begin, InputIterator last, OutputIterator res, Predicate pred, UnaryOp op)
		{
			for (; begin != last; ++begin) {
				if (pred(*begin))
					*res++ = op(*begin);
			}

			return res;
		}
	}
}

#endif /* HYPER_UTILS_ALGORITHM_HH_ */
