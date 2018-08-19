#include "downloadthread.h"

#include "logger.h"

namespace newsboat {

downloadthread::downloadthread(Reloader* r, std::vector<int>* idxs)
	: reloader(r)
{
	if (idxs)
		indexes = *idxs;
}

downloadthread::~downloadthread() {}

void downloadthread::operator()()
{
	/*
	 * the downloadthread class drives the reload-all process.
	 * A downloadthread is spawned whenever "reload all" is invoked, and
	 * whenever an auto-reload comes up.
	 */
	LOG(level::DEBUG,
		"downloadthread::run: inside downloadthread, reloading all "
		"feeds...");
	if (reloader->trylock_reload_mutex()) {
		if (indexes.size() == 0) {
			reloader->reload_all();
		} else {
			reloader->reload_indexes(indexes);
		}
		reloader->unlock_reload_mutex();
	}
}

} // namespace newsboat
