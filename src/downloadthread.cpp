#include <downloadthread.h>

namespace noos
{

downloadthread::downloadthread(controller * c) : ctrl(c) {
}

downloadthread::~downloadthread() {
}

void downloadthread::run() {
	ctrl->reload_all();
	ctrl->unlock_reload_mutex();
	this->detached_exit();
}

}
