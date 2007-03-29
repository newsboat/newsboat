#include <downloadthread.h>
#include <logger.h>

namespace newsbeuter
{

downloadthread::downloadthread(controller * c) : ctrl(c) {
}

downloadthread::~downloadthread() {
}

void downloadthread::run() {
	GetLogger().log(LOG_DEBUG, "downloadthread::run: inside downloadthread, reloading all feeds...");
	ctrl->reload_all();
	ctrl->unlock_reload_mutex();
	this->detach();
}

}
