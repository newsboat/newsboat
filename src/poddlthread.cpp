#include <poddlthread.h>

namespace podbeuter {

poddlthread::poddlthread(download * dl_) : dl(dl_) {
}

poddlthread::~poddlthread() {
}

void poddlthread::run() {
	dl->set_status(DL_DOWNLOADING);
	for (int i=0;i<=100;i+=10) {
		if (i > 0)
			::sleep(1);
		dl->set_progress(i, 100);
	}
	dl->set_status(DL_FINISHED);
}

}
