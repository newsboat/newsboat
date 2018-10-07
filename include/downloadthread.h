#ifndef NEWSBOAT_DOWNLOADTHREAD_H_
#define NEWSBOAT_DOWNLOADTHREAD_H_

#include <thread>

#include "reloader.h"

namespace newsboat {

class DownloadThread {
public:
	DownloadThread(Reloader& r, std::vector<int>* idxs = 0);
	virtual ~DownloadThread();
	void operator()();

private:
	Reloader& reloader;
	std::vector<int> indexes;
};

} // namespace newsboat

#endif /* NEWSBOAT_DOWNLOADTHREAD_H_ */
