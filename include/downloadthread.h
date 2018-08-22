#ifndef NEWSBOAT_DOWNLOADTHREAD_H_
#define NEWSBOAT_DOWNLOADTHREAD_H_

#include <thread>

#include "reloader.h"

namespace newsboat {

class downloadthread {
public:
	downloadthread(Reloader& r, std::vector<int>* idxs = 0);
	virtual ~downloadthread();
	void operator()();

private:
	Reloader& reloader;
	std::vector<int> indexes;
};

} // namespace newsboat

#endif /* NEWSBOAT_DOWNLOADTHREAD_H_ */
