#include <reloadthread.h>
#include <logger.h>

using namespace newsbeuter;

reloadthread::reloadthread(controller * c, unsigned int wt_sec) : ctrl(c), oldtime(0), waittime_sec(wt_sec) {
	GetLogger().log(LOG_INFO,"reloadthread: waiting %u seconds between reloads",waittime_sec);
}

reloadthread::~reloadthread() { }

void reloadthread::run() {
	for (;;) {
		oldtime = time(NULL);
		GetLogger().log(LOG_INFO,"reloadthread: starting reload");
		ctrl->start_reload_all_thread();
		::sleep((oldtime + waittime_sec) - time(NULL));
	}
}
