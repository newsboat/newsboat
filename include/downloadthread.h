#ifndef NEWSBOAT_DOWNLOADTHREAD_H_
#define NEWSBOAT_DOWNLOADTHREAD_H_

#include <thread>

#include "controller.h"

namespace newsboat {

class controller;

class downloadthread {
public:
	downloadthread(controller* c, std::vector<int>* idxs = 0);
	virtual ~downloadthread();
	void operator()();

private:
	controller* ctrl;
	std::vector<int> indexes;
};

class reloadrangethread {
public:
	reloadrangethread(controller* c,
		unsigned int start,
		unsigned int end,
		unsigned int size,
		bool unattended);
	void operator()();

private:
	controller* ctrl;
	unsigned int s, e, ss;
	bool u;
};

} // namespace newsboat

#endif /* NEWSBOAT_DOWNLOADTHREAD_H_ */
