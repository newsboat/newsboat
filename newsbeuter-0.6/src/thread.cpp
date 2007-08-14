#include <thread.h>
#include <exception.h>
#include <logger.h>

namespace newsbeuter {


thread::thread() {
}

thread::~thread() { 

}

void thread::start() {
	int rc = pthread_create(&pt, 0, (void *(*)(void*))run_thread, this);
	GetLogger().log(LOG_DEBUG, "thread::start: created new thread %d rc = %d", pt, rc);
	if (rc != 0) {
		throw exception(rc);
	}
}

void thread::join() {
	pthread_join(pt, NULL);
}

void thread::detach() {
	GetLogger().log(LOG_DEBUG, "thread::detach: detaching thread %d", pt);
	pthread_detach(pt);
}

void * run_thread(thread * p) {
	thread * t = p;
	GetLogger().log(LOG_DEBUG, "run_thread: p = %p", p);
	t->run();
	delete t;
	return 0;
}

void thread::cleanup(thread * p) {
	delete p;
}

}
