#ifndef NEWSBOAT_RELOADRANGETHREAD_H_
#define NEWSBOAT_RELOADRANGETHREAD_H_

#include <thread>

#include "reloader.h"

namespace newsboat {

class ReloadRangeThread {
public:
	ReloadRangeThread(Reloader& r,
		unsigned int start,
		unsigned int end,
		bool unattended);
	void operator()();

private:
	Reloader& reloader;
	unsigned int start;
	unsigned int end;
	bool unattended;
};

} // namespace newsboat

#endif /* NEWSBOAT_RELOADRANGETHREAD_H_ */
