#include <mutex.h>
#include <exception.h>

#include <cerrno>

using namespace newsbeuter;

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
	int rc = pthread_mutex_lock(&mtx);
	if (rc != 0) {
		throw exception(rc);
	}
}

void mutex::unlock() {
	int rc = pthread_mutex_unlock(&mtx);
	if (rc != 0) {
		throw exception(rc);
	}
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
