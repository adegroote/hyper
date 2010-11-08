#ifndef HYPER_MODEL_CONCURRENT_QUEUE
#define HYPER_MODEL_CONCURRENT_QUEUE

#include <queue>

#include <boost/thread/locks.hpp>

namespace hyper {
	namespace model {

		template<typename T, typename Sequence = std::deque<T> >
		class concurrent_queue
		{
			private:
				std::queue<T, Sequence> q;
				mutable boost::mutex m;
				boost::condition_variable cond;

			public:

				void push(const T& t)
				{
					boost::mutex::scoped_lock lock(m);
					q.push(t);
					cond.notify_one();
				}

				bool empty() const
				{
					boost::mutex::scoped_lock lock(m);
					return q.empty();
				}

				bool try_pop(T& t)
				{
					boost::mutex::scoped_lock lock(m);
					if(q.empty())
						return false;

					t = q.front();
					q.pop();
					return true;
				}

				void wait_and_pop(T& t)
				{
					boost::mutex::scoped_lock lock(m);
					while(q.empty())
						cond.wait(lock);

					t = q.front();
					q.pop();
				}
		};
	}
}


#endif /* HYPER_MODEL_CONCURRENT_QUEUE */
