#ifndef NEWSBOAT_DOWNLOADTHREAD_H_
#define NEWSBOAT_DOWNLOADTHREAD_H_

#include <thread>

#include "reloader.h"

namespace newsboat {

class downloadthread {
public:
	downloadthread(Reloader* r, std::vector<int>* idxs = 0);
	virtual ~downloadthread();
	void operator()();

private:
	Reloader* reloader;
	std::vector<int> indexes;
};

class reloadrangethread {
public:
	reloadrangethread(Reloader* r,
		unsigned int start,
		unsigned int end,
		unsigned int size,
		bool unattended);
	void operator()();

private:
	Reloader* reloader;
	unsigned int s, e, ss;
	bool u;
};

} // namespace newsboat

#endif /* NEWSBOAT_DOWNLOADTHREAD_H_ */
