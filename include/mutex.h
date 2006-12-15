#ifndef AK_MUTEX__H
#define AK_MUTEX__H

#include <pthread.h>

namespace noos {

class mutex {
	public:
		mutex();
		~mutex();
		void lock();
		void unlock();
		bool trylock();

	private:
		pthread_mutex_t mtx;

	friend class condition;
};


}

#endif
