#ifndef MARKREADTHREAD_H_
#define MARKREADTHREAD_H_

#include <thread>
#include <ttrss_api.h>

namespace newsbeuter
{
	
class controller;

class markreadthread
{
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

#endif /*MARKREADTHREAD_H_*/
