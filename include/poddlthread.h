#ifndef PODBOAT_PODDLTHREAD_H_
#define PODBOAT_PODDLTHREAD_H_

#include <chrono>
#include <fstream>
#include <memory>
#include <time.h>

#include "configcontainer.h"
#include "download.h"

namespace Podboat {

class PodDlThread {
public:
	PodDlThread(Download* dl_, Newsboat::ConfigContainer& c);
	virtual ~PodDlThread();
	size_t write_data(void* buffer, size_t size, size_t nmemb);
	int progress(double dlnow, double dltotal);
	void operator()();

protected:
	double compute_kbps();

private:
	void run();
	Download* dl;
	std::shared_ptr<std::ofstream> f;
	std::chrono::time_point<std::chrono::steady_clock> tv1;
	std::chrono::time_point<std::chrono::steady_clock> tv2;
	size_t bytecount;
	Newsboat::ConfigContainer& cfg;
};

} // namespace Podboat

#endif /* PODBOAT_PODDLTHREAD_H_ */
