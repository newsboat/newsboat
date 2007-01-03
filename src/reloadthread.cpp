#include <reloadthread.h>

using namespace noos;

reloadthread::reloadthread(controller * c, unsigned int wt_sec) : ctrl(c), oldtime(0), waittime_sec(wt_sec) {
}

reloadthread::~reloadthread() { }

void reloadthread::run() {
	for (;;) {
		oldtime = time(NULL);
		ctrl->start_reload_all_thread();
		::sleep((oldtime + waittime_sec) - time(NULL));
	}
}
