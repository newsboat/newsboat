#ifndef NEWSBOAT_MARKREADTHREAD_H_
#define NEWSBOAT_MARKREADTHREAD_H_

#include <thread>
#include <ttrss_api.h>

namespace newsboat {

class controller;

class markreadthread {
	public:
		markreadthread( ttrss_api* r_api, const std::string& guid, bool read );
		virtual ~markreadthread();
		void operator()();
	private:
		ttrss_api* _r_api;
		const std::string& _guid;
		bool _read;
};

}

#endif /* NEWSBOAT_MARKREADTHREAD_H_ */
