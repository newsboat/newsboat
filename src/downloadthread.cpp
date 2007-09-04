#include <downloadthread.h>
#include <logger.h>

namespace newsbeuter
{

downloadthread::downloadthread(controller * c) : ctrl(c) {
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
		ctrl->reload_all();
		ctrl->unlock_reload_mutex();
	}
	this->detach();
}

}
