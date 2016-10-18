#ifndef LOGGER__H
#define LOGGER__H

#include <fstream>
#include <mutex>
#include <config.h>

namespace newsbeuter {

/* Be sure to update loglevel_str array in src/logger.cpp if you change this
 * enum. */
enum class level {
	NONE = 0,
	USERERROR,
	CRITICAL,
	ERROR,
	WARN,
	INFO,
	DEBUG
};

class logger {
	public:
		static logger &getInstance();

		void set_logfile(const char * logfile);
		void set_errorlogfile(const char * logfile);
		void set_loglevel(level l);
		void log(level l, const char * format, ...);

	private:
		logger();
		logger(const logger &) {}
		logger& operator=(const logger &) {
			return *this;
		}
		~logger() { }

		level curlevel;
		std::mutex logMutex;
		static std::mutex instanceMutex;
		std::fstream f;
		std::fstream ef;
};

}

// see http://kernelnewbies.org/FAQ/DoWhile0
#ifdef NDEBUG
#define LOG(x, ...) do { } while(0)
#else
#define LOG(x, ...) do { logger::getInstance().log(x, __VA_ARGS__); } while(0)
#endif

#endif
