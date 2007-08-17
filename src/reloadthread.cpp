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

		time_t seconds_to_wait = 0;
		if (oldtime + waittime_sec > time(NULL))
			seconds_to_wait = oldtime + waittime_sec - time(NULL);

		while (seconds_to_wait > 0) {
			seconds_to_wait = ::sleep(seconds_to_wait);
		}
	}
}
