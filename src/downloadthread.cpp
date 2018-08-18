#include "downloadthread.h"

#include "logger.h"

namespace newsboat {

downloadthread::downloadthread(controller* c, std::vector<int>* idxs)
	: ctrl(c)
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
	if (ctrl->get_reloader()->trylock_reload_mutex()) {
		if (indexes.size() == 0) {
			ctrl->get_reloader()->reload_all();
		} else {
			ctrl->get_reloader()->reload_indexes(indexes);
		}
		ctrl->get_reloader()->unlock_reload_mutex();
	}
}

reloadrangethread::reloadrangethread(controller* c,
	unsigned int start,
	unsigned int end,
	unsigned int size,
	bool unattended)
	: ctrl(c)
	, s(start)
	, e(end)
	, ss(size)
	, u(unattended)
{
}

void reloadrangethread::operator()()
{
	ctrl->reload_range(s, e, ss, u);
}

} // namespace newsboat
