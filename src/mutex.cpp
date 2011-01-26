#include <mutex.h>
#include <exception.h>
#include <logger.h>

#include <cerrno>

namespace newsbeuter {

mutex::mutex() {
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
	pthread_mutex_init(&mtx, &attr);
}

mutex::~mutex() {
	pthread_mutex_destroy(&mtx);
	pthread_mutexattr_destroy(&attr);
}

void mutex::lock() {
	pthread_mutex_lock(&mtx);
}

void mutex::unlock() {
	pthread_mutex_unlock(&mtx);
}

bool mutex::trylock() {
	int rc = pthread_mutex_trylock(&mtx);
	if (rc != 0) {
		if (EBUSY == rc) {
			return false;
		} else {
			throw exception(rc);
		}
	} else {
		return true;
	}
}

scope_mutex::scope_mutex(mutex * m) : mtx(m) {
	if (mtx) {
		mtx->lock();
	}
}

scope_mutex::~scope_mutex() {
	if (mtx) {
		mtx->unlock();
	}
}

}
