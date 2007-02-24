#ifndef PODBEUTER_PODDLTHREAD__H
#define PODBEUTER_PODDLTHREAD__H

#include <thread.h>
#include <download.h>

namespace podbeuter {

class poddlthread : public newsbeuter::thread {
	public:
		poddlthread(download * dl_);
		virtual ~poddlthread();
	protected:
		virtual void run();
	private:
		download * dl;
};

}


#endif
