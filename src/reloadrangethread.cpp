#include "reloadrangethread.h"

namespace newsboat {

ReloadRangeThread::ReloadRangeThread(Reloader& r,
	unsigned int s,
	unsigned int e,
	bool u)
	: reloader(r)
	, start(s)
	, end(e)
	, unattended(u)
{
}

void ReloadRangeThread::operator()()
{
	reloader.reload_range(start, end, unattended);
}

} // namespace newsboat
