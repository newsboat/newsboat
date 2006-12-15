#include <mutex.h>
#include <exception.h>

#include <cerrno>

using namespace noos;

mutex::mutex() {
	pthread_mutex_init(&mtx, NULL);
}

mutex::~mutex() {
	pthread_mutex_destroy(&mtx);
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
