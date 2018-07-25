#include "reloader.h"

#include <thread>

#include "controller.h"
#include "downloadthread.h"
#include "reloadthread.h"

namespace newsboat {

Reloader::Reloader(controller* c)
	: ctrl(c)
{
}

void Reloader::spawn_reloadthread()
{
	std::thread t{reloadthread(ctrl, ctrl->get_cfg())};
	t.detach();
}

void Reloader::start_reload_all_thread(std::vector<int>* indexes)
{
	LOG(level::INFO, "starting reload all thread");
	std::thread t(downloadthread(ctrl, indexes));
	t.detach();
}

} // namespace newsboat
