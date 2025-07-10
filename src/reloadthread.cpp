#include "reloadthread.h"

#include <cinttypes>
#include <unistd.h>

#include "logger.h"

namespace Newsboat {

ReloadThread::ReloadThread(Controller& c, ConfigContainer& cf)
	: ctrl(c)
	, oldtime(0)
	, waittime_sec(0)
	, suppressed_first(false)
	, cfg(cf)
{
	LOG(Level::INFO,
		"ReloadThread: waiting %" PRIi64 " seconds between reloads",
		// In GCC, `time t` is `long int`, which is at least 32 bits. On
		// x86_64, it's 64 bits. Thus, this cast is either a no-op, or an
		// up-cast which are always safe.
		static_cast<int64_t>(waittime_sec));
}

ReloadThread::~ReloadThread() {}

void ReloadThread::operator()()
{
	for (;;) {
		oldtime = time(nullptr);
		LOG(Level::INFO, "ReloadThread: starting reload");

		waittime_sec = 60 * cfg.get_configvalue_as_int("reload-time");
		if (waittime_sec == 0) {
			waittime_sec = 60;
		}

		if (cfg.get_configvalue_as_bool("auto-reload")) {
			if (suppressed_first) {
				ctrl.get_reloader()->start_reload_all_thread();
			} else {
				suppressed_first = true;
				if (!cfg.get_configvalue_as_bool(
						"suppress-first-reload")) {
					ctrl.get_reloader()->start_reload_all_thread();
				}
			}
		} else {
			// if auto-reload is disabled, we poll every 60 seconds whether it changed.
			waittime_sec = 60;
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

} // namespace Newsboat
