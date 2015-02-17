#ifndef DOWNLOADTHREAD_H_
#define DOWNLOADTHREAD_H_

#include <thread>
#include <controller.h>

namespace newsbeuter
{
	
class controller;

class downloadthread
{
public:
	downloadthread(controller * c, std::vector<int> * idxs = 0);
	virtual ~downloadthread();
	void operator()();
private:
	controller * ctrl;
	std::vector<int> indexes;
};

class reloadrangethread
{
public:
	reloadrangethread(controller * c, unsigned int start, unsigned int end, unsigned int size, bool unattended);
	void operator()();
private:
	controller * ctrl;
	unsigned int s, e, ss;
	bool u;
};

}

#endif /*DOWNLOADTHREAD_H_*/
