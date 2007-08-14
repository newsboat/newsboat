#ifndef PODBEUTER_PODDLTHREAD__H
#define PODBEUTER_PODDLTHREAD__H

#include <thread.h>
#include <download.h>
#include <fstream>

#include <sys/time.h>
#include <time.h>

#include <configcontainer.h>

namespace podbeuter {

class poddlthread : public newsbeuter::thread {
	public:
		poddlthread(download * dl_, newsbeuter::configcontainer *);
		virtual ~poddlthread();
		size_t write_data(void * buffer, size_t size, size_t nmemb);
		int progress(double dlnow, double dltotal);
	protected:
		virtual void run();
		double compute_kbps();
	private:
		download * dl;
		std::ofstream f;
		timeval tv1;
		timeval tv2;
		size_t bytecount;
		newsbeuter::configcontainer * cfg;
};

}


#endif
