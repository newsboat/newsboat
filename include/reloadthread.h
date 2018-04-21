#ifndef NEWSBOAT_RELOADTHREAD_H_
#define NEWSBOAT_RELOADTHREAD_H_

#include <thread>

#include "controller.h"
#include "configcontainer.h"

namespace newsboat {

class reloadthread {
	public:
		reloadthread(controller * c, configcontainer * cf);
		virtual ~reloadthread();
		void operator()();
	private:
		controller * ctrl;
		time_t oldtime;
		time_t waittime_sec;
		bool suppressed_first;
		configcontainer * cfg;
};

}

#endif /* NEWSBOAT_RELOADTHREAD_H_ */
