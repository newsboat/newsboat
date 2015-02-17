#include <reloadthread.h>
#include <logger.h>
#include <unistd.h>

namespace newsbeuter {

reloadthread::reloadthread(controller * c, configcontainer * cf) : ctrl(c), oldtime(0), waittime_sec(0), suppressed_first(false), cfg(cf) {
	LOG(LOG_INFO,"reloadthread: waiting %u seconds between reloads",waittime_sec);
}

reloadthread::~reloadthread() { }

void reloadthread::operator()() {
	for (;;) {
		oldtime = time(NULL);
		LOG(LOG_INFO,"reloadthread: starting reload");

		waittime_sec = 60 * cfg->get_configvalue_as_int("reload-time");
		if (waittime_sec == 0)
			waittime_sec = 60;

		if (cfg->get_configvalue_as_bool("auto-reload")) {
			if (suppressed_first) {
				ctrl->start_reload_all_thread();
			} else {
				suppressed_first = true;
				if (!cfg->get_configvalue_as_bool("suppress-first-reload")) {
					ctrl->start_reload_all_thread();
				}
			}
		} else {
			waittime_sec = 60; // if auto-reload is disabled, we poll every 60 seconds whether it changed.
		}

		time_t seconds_to_wait = 0;
		if ((oldtime + waittime_sec) > time(NULL))
			seconds_to_wait = oldtime + waittime_sec - time(NULL);

		while (seconds_to_wait > 0) {
			seconds_to_wait = ::sleep(seconds_to_wait);
		}
	}
}

}
