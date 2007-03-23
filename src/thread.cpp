#include <thread.h>
#include <exception.h>

namespace newsbeuter {


thread::thread() {
}

thread::~thread() { 

}

void thread::start() {
	int rc = pthread_create(&pt, 0, (void *(*)(void*))run_thread, this);
	if (rc != 0) {
		throw exception(rc);
	}
}

void thread::join() {
	pthread_join(pt, NULL);
}

void thread::detach() {
	pthread_detach(pt);
}

void * run_thread(thread * p) {
	thread * t = p;
	t->run();
	delete t;
	return 0;
}

void thread::cleanup(thread * p) {
	delete p;
}

}
