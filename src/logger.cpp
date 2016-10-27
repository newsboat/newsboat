#include <logger.h>
#include <stdarg.h>
#include <exception.h>
#include <cerrno>

namespace newsbeuter {
std::mutex logger::instanceMutex;

logger::logger() : curlevel(LOG_NONE) { }

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

void logger::set_errorlogfile(const char * logfile) {
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
	if (LOG_NONE == curlevel) {
		curlevel = LOG_USERERROR;
	}
}

void logger::set_loglevel(loglevel level) {
	std::lock_guard<std::mutex> lock(logMutex);
	curlevel = level;
	if (curlevel == LOG_NONE)
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
