#include "reloadrangethread.h"

namespace newsboat {

ReloadRangeThread::ReloadRangeThread(Reloader& r,
	unsigned int s,
	unsigned int e,
	unsigned int ss,
	bool u)
	: reloader(r)
	, start(s)
	, end(e)
	, size(ss)
	, unattended(u)
{}

void ReloadRangeThread::operator()()
{
	reloader.reload_range(start, end, size, unattended);
}

} // namespace newsboat
