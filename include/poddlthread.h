#ifndef PODBEUTER_PODDLTHREAD__H
#define PODBEUTER_PODDLTHREAD__H

#include <thread.h>
#include <download.h>

namespace podbeuter {

class poddlthread : public newsbeuter::thread {
	public:
		poddlthread(download * dl_);
		virtual ~poddlthread();
		size_t write_data(void * buffer, size_t size, size_t nmemb);
		int progress(double dlnow, double dltotal);
	protected:
		virtual void run();
	private:
		download * dl;
};

}


#endif
