#include <downloadthread.h>
#include <logger.h>

namespace newsbeuter
{

downloadthread::downloadthread(controller * c, std::vector<int> * idxs) : ctrl(c) {
	if (idxs)
		indexes = *idxs;
}

downloadthread::~downloadthread() {
}

void downloadthread::run() {
	/*
	 * the downloadthread class drives the reload-all process.
	 * A downloadthread is spawned whenever "reload all" is invoked, and whenever an auto-reload
	 * comes up.
	 */
	GetLogger().log(LOG_DEBUG, "downloadthread::run: inside downloadthread, reloading all feeds...");
	if (ctrl->trylock_reload_mutex()) {
		if (indexes.size() == 0) {
			ctrl->reload_all();
		} else {
			ctrl->reload_indexes(indexes);
		}
		ctrl->unlock_reload_mutex();
	}
	this->detach();
}

reloadrangethread::reloadrangethread(controller * c, unsigned int start, unsigned int end, unsigned int size, bool unattended) : ctrl(c), s(start), e(end), ss(size), u(unattended)
{
}

void reloadrangethread::run() {
	ctrl->reload_range(s, e, ss, u);
}

}
