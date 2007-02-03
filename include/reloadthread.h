#ifndef RELOADTHREAD_H_
#define RELOADTHREAD_H_

#include <thread.h>
#include <controller.h>

namespace newsbeuter
{

class reloadthread : public thread
{
public:
	reloadthread(controller * c, unsigned int wt_sec);
	virtual ~reloadthread();
protected:
	virtual void run();
private:
	controller * ctrl;
	time_t oldtime;
	unsigned int waittime_sec;
};

}

#endif
