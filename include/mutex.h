#ifndef AK_MUTEX__H
#define AK_MUTEX__H

#include <pthread.h>

namespace newsbeuter {

class mutex {
	public:
		mutex();
		~mutex();
		void lock();
		void unlock();
		bool trylock();

	private:
		pthread_mutex_t mtx;
		pthread_mutexattr_t attr;

	friend class condition;
};

class scope_mutex {
	public:
		scope_mutex(mutex * m);
		~scope_mutex();
	private:
		mutex * mtx;
};

}

#endif
