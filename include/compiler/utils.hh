
#ifndef _COMPILER_UTIL_HH_
#define _COMPILER_UTIL_HH_

#include <algorithm>
#include <string>

namespace hyper {
	namespace compiler {
		std::string read_from_file(const std::string&);

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
		copy_if(InputIterator begin, InputIterator last, OutputIterator res, Predicate p)
		{
			return std::remove_copy_if(begin, last, res, std::not1(p));
		}

		/*
		 * Replace all occurrence of @pattern by @replace in @str
		 */
		std::string replace_by(const std::string& , const std::string& pattern,	
													const std::string& replace);
	}
}

#endif /* _COMPILER_UTIL_HH_ */
