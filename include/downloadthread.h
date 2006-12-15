#ifndef DOWNLOADTHREAD_H_
#define DOWNLOADTHREAD_H_

#include <thread.h>
#include <controller.h>

namespace noos
{
	
class controller;

class downloadthread : public thread
{
public:
	downloadthread(controller * c);
	virtual ~downloadthread();
protected:
	virtual void run();
private:
	controller * ctrl;
};

}

#endif /*DOWNLOADTHREAD_H_*/
