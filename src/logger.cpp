#include <logger.h>
#include <stdarg.h>
#include <exception.h>
#include <cerrno>

namespace newsbeuter {
std::mutex logger::instanceMutex;

logger::logger() : curlevel(LOG_NONE) { }

void logger::set_logfile(const char * logfile) {
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

const char * loglevel_str[] = { "NONE", "USERERROR", "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG" };

void logger::log(loglevel level, const char * format, ...) {
	/*
	 * This function checks the loglevel, creates the error message, and then
	 * writes it to the debug logfile and to the error logfile (if applicable).
	 */
	std::lock_guard<std::mutex> lock(logMutex);
	if (level <= curlevel && curlevel > LOG_NONE && (f.is_open() || ef.is_open())) {
		char * buf, * logmsgbuf;
		char date[128];
		time_t t = time(NULL);
		struct tm * stm = localtime(&t);
		strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S",stm);
		if (curlevel > LOG_DEBUG)
			curlevel = LOG_DEBUG;

		va_list ap;
		va_start(ap, format);
		unsigned int len = vsnprintf(NULL,0,format,ap);
		va_end(ap);

		va_start(ap, format);
		logmsgbuf = new char[len + 1];
		vsnprintf(logmsgbuf, len + 1, format, ap);
		va_end(ap);

		len = snprintf(NULL, 0, "[%s] %s: %s",date, loglevel_str[level], logmsgbuf);
		buf = new char[len + 1];
		snprintf(buf,len + 1,"[%s] %s: %s",date, loglevel_str[level], logmsgbuf);

		if (f.is_open()) {
			f << buf << std::endl;
		}

		if (LOG_USERERROR == level && ef.is_open()) {
			snprintf(buf, len + 1, "[%s] %s", date, logmsgbuf);
			ef << buf << std::endl;
			ef.flush();
		}

		delete[] buf;
		delete[] logmsgbuf;
	}
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
