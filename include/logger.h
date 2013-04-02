#ifndef LOGGER__H
#define LOGGER__H

#include <fstream>
#include <mutex.h>
#include <config.h>

namespace newsbeuter {

	enum loglevel { LOG_NONE = 0, LOG_USERERROR, LOG_CRITICAL, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG };


class logger {
	public:
		logger();
		~logger() { }
		void set_logfile(const char * logfile);
		void set_errorlogfile(const char * logfile);
		void set_loglevel(loglevel level);
		void log(loglevel level, const char * format, ...);

	private:
		loglevel curlevel;
		mutex mtx;
		std::fstream f;
		std::fstream ef;
};

logger& GetLogger();

}

#ifdef NO_DEBUG
#define LOG(x, ...) do { } while(0)
#else
#define LOG(x, ...) do { GetLogger().log(x, __VA_ARGS__); } while(0)
#endif

#endif
