#ifndef PODBEUTER_PODDLTHREAD__H
#define PODBEUTER_PODDLTHREAD__H

#include <thread>
#include <download.h>
#include <fstream>

#include <sys/time.h>
#include <time.h>

#include <configcontainer.h>

namespace podbeuter {

class poddlthread {
	public:
		poddlthread(download * dl_, newsbeuter::configcontainer *);
		virtual ~poddlthread();
		size_t write_data(void * buffer, size_t size, size_t nmemb);
		int progress(double dlnow, double dltotal);
		void operator()();
	protected:
		double compute_kbps();
	private:
		void mkdir_p(const char * file);
		download * dl;
		std::ofstream *f;
		timeval tv1;
		timeval tv2;
		size_t bytecount;
		newsbeuter::configcontainer * cfg;
};

}


#endif
