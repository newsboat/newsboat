#include "downloadthread.h"

#include "logger.h"

namespace newsboat {

DownloadThread::DownloadThread(Reloader& r, const std::vector<int>& idxs,
	bool notify_on_finish_)
	: reloader(r)
	, indexes(idxs)
	, notify_on_finish(notify_on_finish_)
{
}

DownloadThread::~DownloadThread() {}

void DownloadThread::operator()()
{
	/*
	 * the DownloadThread class drives the reload-all process.
	 * A DownloadThread is spawned whenever "reload all" is invoked, and
	 * whenever an auto-reload comes up.
	 */
	LOG(Level::DEBUG,
		"DownloadThread::run: inside DownloadThread, reloading all "
		"feeds...");
	if (reloader.trylock_reload_mutex()) {
		if (indexes.size() == 0) {
			reloader.reload_all();
		} else {
			reloader.reload_indexes(indexes, notify_on_finish);
		}
		reloader.unlock_reload_mutex();
	}
}

} // namespace newsboat
