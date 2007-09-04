#include <logger.h>
#include <stdarg.h>
#include <exception.h>
#include <cerrno>

namespace newsbeuter {

logger::logger() : curlevel(LOG_NONE) { }

void logger::set_logfile(const char * logfile) {
	/*
	 * This sets the filename of the debug logfile
	 */
	mtx.lock();
	if (f.is_open())
		f.close();
	f.open(logfile, std::fstream::out);
	if (!f.is_open()) {
		mtx.unlock();
		throw exception(errno); // the question is whether f.open() sets errno...
	}
	mtx.unlock();
}

void logger::set_errorlogfile(const char * logfile) {
	/*
	 * This sets the filename of the error logfile, i.e. the one that can be configured to be generated.
	 */
	mtx.lock();
	if (ef.is_open())
		ef.close();
	ef.open(logfile, std::fstream::out);
	if (!ef.is_open()) {
		mtx.unlock();
		throw exception(errno);
	}
	if (LOG_NONE == curlevel) {
		curlevel = LOG_USERERROR;
	}
	mtx.unlock();
}

void logger::set_loglevel(loglevel level) {
	mtx.lock();
	curlevel = level;
	if (curlevel == LOG_NONE)
		f.close();
	mtx.unlock();
}

const char * loglevel_str[] = { "NONE", "USERERROR", "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG" };

void logger::log(loglevel level, const char * format, ...) {
	/*
	 * This function checks the loglevel, creates the error message, and then
	 * writes it to the debug logfile and to the error logfile (if applicable).
	 */
	mtx.lock();
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
		logmsgbuf = (char *)alloca(len + 1);
		vsnprintf(logmsgbuf, len + 1, format, ap);

		len = snprintf(NULL, 0, "[%s] %s: %s",date, loglevel_str[level], logmsgbuf);
		buf = (char *)alloca(len + 1);
		snprintf(buf,len + 1,"[%s] %s: %s",date, loglevel_str[level], logmsgbuf);

		if (f.is_open()) {
			f << buf << std::endl;
		}

		if (LOG_USERERROR == level && ef.is_open()) {
			snprintf(buf, len + 1, "[%s] %s", date, logmsgbuf);
			ef << buf << std::endl;
			ef.flush();
		}

	}
	mtx.unlock();
}

logger& GetLogger() {
	/*
	 * This is the global logger that everyone uses
	 */
	static logger theLogger;
	return theLogger;
}

}
