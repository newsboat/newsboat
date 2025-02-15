#ifndef NEWSBOAT_RELOADTHREAD_H_
#define NEWSBOAT_RELOADTHREAD_H_

#include "configcontainer.h"
#include "controller.h"

namespace newsboat {

class ReloadThread {
public:
	ReloadThread(Controller& c, ConfigContainer& cf);
	virtual ~ReloadThread();
	void operator()();

private:
	Controller& ctrl;
	time_t oldtime;
	time_t waittime_sec;
	bool suppressed_first;
	ConfigContainer& cfg;
};

} // namespace newsboat

#endif /* NEWSBOAT_RELOADTHREAD_H_ */
