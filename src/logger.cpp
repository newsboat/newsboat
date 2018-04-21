#include "logger.h"

#include <stdarg.h>
#include <cerrno>

#include "exception.h"

namespace newsboat {
std::mutex logger::instanceMutex;

logger::logger() : curlevel(level::NONE) { }

void logger::set_logfile(const std::string& logfile) {
	/*
	 * This sets the filename of the debug logfile
	 */
	std::lock_guard<std::mutex> lock(logMutex);
	if (f.is_open())
		f.close();
	f.open(logfile, std::fstream::out);
	if (!f.is_open()) {
		throw exception(errno); // the question is whether f.open() sets errno...
	}
}

void logger::set_errorlogfile(const std::string& logfile) {
	/*
	 * This sets the filename of the error logfile, i.e. the one that can be configured to be generated.
	 */
	std::lock_guard<std::mutex> lock(logMutex);
	if (ef.is_open())
		ef.close();
	ef.open(logfile, std::fstream::out);
	if (!ef.is_open()) {
		throw exception(errno);
	}
	if (level::NONE == curlevel) {
		curlevel = level::USERERROR;
	}
}

void logger::set_loglevel(level l) {
	std::lock_guard<std::mutex> lock(logMutex);
	curlevel = l;
	if (curlevel == level::NONE)
		f.close();
}

logger &logger::getInstance() {
	/*
	 * This is the global logger that everyone uses
	 */
	std::lock_guard<std::mutex> lock(instanceMutex);
	static logger theLogger;
	return theLogger;
}

}
