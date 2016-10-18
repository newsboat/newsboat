#include <logger.h>
#include <stdarg.h>
#include <exception.h>
#include <cerrno>


namespace newsbeuter {
std::mutex logger::instanceMutex;

logger::logger() : curlevel(level::NONE) { }

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

/* Be sure to update enum class level in include/logger.h if you change this
 * array. */
const char * loglevel_str[] = {
	"NONE",
	"USERERROR",
	"CRITICAL",
	"ERROR",
	"WARNING",
	"INFO",
	"DEBUG"
};

void logger::log(level l, const char * format, ...) {
	/*
	 * This function checks the loglevel, creates the error message, and then
	 * writes it to the debug logfile and to the error logfile (if applicable).
	 */
	std::lock_guard<std::mutex> lock(logMutex);
	if (l <= curlevel && curlevel > level::NONE && (f.is_open() || ef.is_open())) {
		char * buf, * logmsgbuf;
		char date[128];
		time_t t = time(nullptr);
		struct tm * stm = localtime(&t);
		strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S",stm);
		if (curlevel > level::DEBUG)
			curlevel = level::DEBUG;

		va_list ap;
		va_start(ap, format);
		unsigned int len = vsnprintf(nullptr,0,format,ap);
		va_end(ap);

		va_start(ap, format);
		logmsgbuf = new char[len + 1];
		vsnprintf(logmsgbuf, len + 1, format, ap);
		va_end(ap);

		len = snprintf(
				nullptr, 0, "[%s] %s: %s",
				date, loglevel_str[static_cast<int>(l)], logmsgbuf);
		buf = new char[len + 1];
		snprintf(
				buf, len + 1, "[%s] %s: %s",
				date, loglevel_str[static_cast<int>(l)], logmsgbuf);

		if (f.is_open()) {
			f << buf << std::endl;
		}

		if (level::USERERROR == l && ef.is_open()) {
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
