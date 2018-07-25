#include "reloader.h"

#include <thread>

#include "controller.h"
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

} // namespace newsboat
