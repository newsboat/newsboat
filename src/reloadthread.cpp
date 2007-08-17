#include <reloadthread.h>
#include <logger.h>

using namespace newsbeuter;

reloadthread::reloadthread(controller * c, unsigned int wt_sec, configcontainer * cf) : ctrl(c), oldtime(0), waittime_sec(wt_sec), suppressed_first(false), cfg(cf) {
	GetLogger().log(LOG_INFO,"reloadthread: waiting %u seconds between reloads",waittime_sec);
}

reloadthread::~reloadthread() { }

void reloadthread::run() {
	for (;;) {
		oldtime = time(NULL);
		GetLogger().log(LOG_INFO,"reloadthread: starting reload");

		if (suppressed_first) {
			ctrl->start_reload_all_thread();
		} else {
			suppressed_first = true;
			if (!cfg->get_configvalue_as_bool("suppress-first-reload")) {
				ctrl->start_reload_all_thread();
			}
		}

		int err = 0;
		do {
			time_t curtime = time(NULL);
			if ((oldtime + waittime_sec) > curtime) {
				if (::sleep((oldtime + waittime_sec) - curtime)>0 && errno == EINTR)
					err = 1;
			}
		} while (err);
	}
}
