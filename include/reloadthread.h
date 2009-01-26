#ifndef RELOADTHREAD_H_
#define RELOADTHREAD_H_

#include <thread.h>
#include <controller.h>
#include <configcontainer.h>

namespace newsbeuter
{

class reloadthread : public thread
{
public:
	reloadthread(controller * c, configcontainer * cf);
	virtual ~reloadthread();
protected:
	virtual void run();
private:
	controller * ctrl;
	time_t oldtime;
	time_t waittime_sec;
	bool suppressed_first;
	configcontainer * cfg;
};

}

#endif
