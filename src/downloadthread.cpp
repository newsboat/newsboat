#include "downloadthread.h"

#include "logger.h"

namespace newsboat {

Downloadthread::Downloadthread(Reloader& r, std::vector<int>* idxs)
	: reloader(r)
{
	if (idxs) {
		indexes = *idxs;
	}
}

Downloadthread::~Downloadthread() {}

void Downloadthread::operator()()
{
	/*
	 * the Downloadthread class drives the reload-all process.
	 * A Downloadthread is spawned whenever "reload all" is invoked, and
	 * whenever an auto-reload comes up.
	 */
	LOG(Level::DEBUG,
		"Downloadthread::run: inside Downloadthread, reloading all "
		"feeds...");
	if (reloader.trylock_reload_mutex()) {
		if (indexes.size() == 0) {
			reloader.reload_all();
		} else {
			reloader.reload_indexes(indexes);
		}
		reloader.unlock_reload_mutex();
	}
}

} // namespace newsboat
