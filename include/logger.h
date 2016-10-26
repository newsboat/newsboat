#ifndef LOGGER__H
#define LOGGER__H

#include <fstream>
#include <mutex>
#include <config.h>

#include <strprintf.h>

namespace newsbeuter {

enum loglevel { LOG_NONE = 0, LOG_USERERROR, LOG_CRITICAL, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG };


class logger {
	public:
		static logger &getInstance();

		void set_logfile(const char * logfile);
		void set_errorlogfile(const char * logfile);
		void set_loglevel(loglevel level);

		template<typename... Args>
		void log(loglevel level, const std::string& format, Args... args) {
			const char * loglevel_str[] =
				{ "NONE", "USERERROR", "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG" };
			/*
			 * This function checks the loglevel, creates the error message, and then
			 * writes it to the debug logfile and to the error logfile (if applicable).
			 */
			std::lock_guard<std::mutex> lock(logMutex);
			if (level <= curlevel && curlevel > LOG_NONE && (f.is_open() || ef.is_open())) {
				if (curlevel > LOG_DEBUG)
					curlevel = LOG_DEBUG;

				char date[128];
				time_t t = time(nullptr);
				struct tm * stm = localtime(&t);
				strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", stm);

				auto logmsgbuf = strprintf::fmt(format, args...);
				auto buf = strprintf::fmt("[%s] %s: %s", date, loglevel_str[level], logmsgbuf);

				if (f.is_open()) {
					f << buf << std::endl;
				}

				if (LOG_USERERROR == level && ef.is_open()) {
					buf = strprintf::fmt("[%s] %s", date, logmsgbuf);
					ef << buf << std::endl;
					ef.flush();
				}
			}
		}


	private:
		logger();
		logger(const logger &) {}
		logger& operator=(const logger &) {
			return *this;
		}
		~logger() { }

		loglevel curlevel;
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
