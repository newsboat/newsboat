#include "reloadrangethread.h"

namespace newsboat {

reloadrangethread::reloadrangethread(Reloader* r,
	unsigned int start,
	unsigned int end,
	unsigned int size,
	bool unattended)
	: reloader(r)
	, s(start)
	, e(end)
	, ss(size)
	, u(unattended)
{
}

void reloadrangethread::operator()()
{
	reloader->reload_range(s, e, ss, u);
}

} // namespace newsboat
