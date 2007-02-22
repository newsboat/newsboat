#include <logger.h>
#include <stdarg.h>
#include <exception.h>
#include <cerrno>

namespace newsbeuter {

logger::logger() : curlevel(LOG_NONE) { }

void logger::set_logfile(const char * logfile) {
	mtx.lock();
	if (f.is_open())
		f.close();
	f.open(logfile, std::fstream::out);
	if (!f.is_open()) {
		throw exception(errno); // the question is whether f.open() sets errno...
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

char * loglevel_str[] = { "NONE", "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG" };

void logger::log(loglevel level, const char * format, ...) {
	mtx.lock();
	if (level <= curlevel && curlevel > LOG_NONE && f.is_open()) {
		char buf[2048], logmsgbuf[2048];
		char date[128];
		time_t t = time(NULL);
		struct tm * stm = localtime(&t);
		strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S",stm);
		if (curlevel > LOG_DEBUG)
			curlevel = LOG_DEBUG;

		va_list ap;
		va_start(ap, format);
		vsnprintf(logmsgbuf,sizeof(logmsgbuf),format,ap);
		snprintf(buf,sizeof(buf),"[%s] %s: %s",date, loglevel_str[level], logmsgbuf);
		f << buf << std::endl;
	}
	mtx.unlock();
}

logger& GetLogger() {
	static logger theLogger;
	return theLogger;
}

}
