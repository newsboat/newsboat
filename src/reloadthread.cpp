#include "reloadthread.h"

#include <unistd.h>

#include "logger.h"

namespace newsboat {

ReloadThread::ReloadThread(Controller* c, ConfigContainer* cf)
	: ctrl(c)
	, oldtime(0)
	, waittime_sec(0)
	, suppressed_first(false)
	, cfg(cf)
{
	LOG(Level::INFO,
		"ReloadThread: waiting %u seconds between reloads",
		waittime_sec);
}

ReloadThread::~ReloadThread() {}

void ReloadThread::operator()()
{
	for (;;) {
		oldtime = time(nullptr);
		LOG(Level::INFO, "ReloadThread: starting reload");

		waittime_sec = 60 * cfg->get_configvalue_as_int("reload-time");
		if (waittime_sec == 0)
			waittime_sec = 60;

		if (cfg->get_configvalue_as_bool("auto-reload")) {
			if (suppressed_first) {
				ctrl->get_reloader()->start_reload_all_thread();
			} else {
				suppressed_first = true;
				if (!cfg->get_configvalue_as_bool(
					    "suppress-first-reload")) {
					ctrl->get_reloader()
						->start_reload_all_thread();
				}
			}
		} else {
			waittime_sec = 60; // if auto-reload is disabled, we
					   // poll every 60 seconds whether it
					   // changed.
		}

		time_t seconds_to_wait = 0;
		if ((oldtime + waittime_sec) > time(nullptr))
			seconds_to_wait =
				oldtime + waittime_sec - time(nullptr);

		while (seconds_to_wait > 0) {
			seconds_to_wait = ::sleep(seconds_to_wait);
		}
	}
}

} // namespace newsboat
