#ifndef PODBOAT_PODDLTHREAD_H_
#define PODBOAT_PODDLTHREAD_H_

#include <fstream>
#include <memory>
#include <sys/time.h>
#include <thread>
#include <time.h>

#include "configcontainer.h"
#include "download.h"

namespace podboat {

class poddlthread {
public:
	poddlthread(download* dl_, newsboat::configcontainer*);
	virtual ~poddlthread();
	size_t write_data(void* buffer, size_t size, size_t nmemb);
	int progress(double dlnow, double dltotal);
	void operator()();

protected:
	double compute_kbps();

private:
	void run();
	download* dl;
	std::shared_ptr<std::ofstream> f;
	timeval tv1;
	timeval tv2;
	size_t bytecount;
	newsboat::configcontainer* cfg;
};

} // namespace podboat

#endif /* PODBOAT_PODDLTHREAD_H_ */
