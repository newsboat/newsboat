#ifndef NEWSBOAT_DOWNLOADTHREAD_H_
#define NEWSBOAT_DOWNLOADTHREAD_H_

#include <thread>

#include "reloader.h"

namespace newsboat {

class DownloadThread {
public:
	DownloadThread(Reloader& r, const std::vector<int>& idxs = {}, bool
		notify_on_finish_ = false);
	virtual ~DownloadThread();
	void operator()();

private:
	Reloader& reloader;
	std::vector<int> indexes;
	const bool notify_on_finish;
};

} // namespace newsboat

#endif /* NEWSBOAT_DOWNLOADTHREAD_H_ */
