#ifndef NEWSBOAT_LOGGER_H_
#define NEWSBOAT_LOGGER_H_

#include <fstream>
#include <mutex>

#include "config.h"
#include "strprintf.h"

namespace newsboat {

/* Be sure to update loglevel_str array in src/logger.cpp if you change this
 * enum. */
enum class Level { NONE = 0, USERERROR, CRITICAL, ERROR, WARN, INFO, DEBUG };

class Logger {
public:
	static Logger& getInstance();

	void set_logfile(const std::string& logfile);
	void set_errorlogfile(const std::string& logfile);
	void set_loglevel(Level l);

	template<typename... Args>
	void log(Level l, const std::string& format, Args... args)
	{
		const char* loglevel_str[] = {"NONE",
			"USERERROR",
			"CRITICAL",
			"ERROR",
			"WARNING",
			"INFO",
			"DEBUG"};
		/*
		 * This function checks the loglevel, creates the error message,
		 * and then writes it to the debug logfile and to the error
		 * logfile (if applicable).
		 */
		std::lock_guard<std::mutex> lock(logMutex);
		if (l <= curlevel && curlevel > Level::NONE &&
			(f.is_open() || ef.is_open())) {
			if (curlevel > Level::DEBUG)
				curlevel = Level::DEBUG;

			char date[128];
			time_t t = time(nullptr);
			struct tm* stm = localtime(&t);
			strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", stm);

			auto logmsgbuf = StrPrintf::fmt(format, args...);
			auto buf = StrPrintf::fmt("[%s] %s: %s",
				date,
				loglevel_str[static_cast<int>(l)],
				logmsgbuf);

			if (f.is_open()) {
				f << buf << std::endl;
			}

			if (Level::USERERROR == l && ef.is_open()) {
				buf = StrPrintf::fmt(
					"[%s] %s", date, logmsgbuf);
				ef << buf << std::endl;
				ef.flush();
			}
		}
	}

private:
	Logger();
	Logger(const Logger&) {}
	Logger& operator=(const Logger&)
	{
		return *this;
	}
	~Logger() {}

	Level curlevel;
	std::mutex logMutex;
	static std::mutex instanceMutex;
	std::fstream f;
	std::fstream ef;
};

} // namespace newsboat

// see http://kernelnewbies.org/FAQ/DoWhile0
#ifdef NDEBUG
#define LOG(x, ...) \
	do {        \
	} while (0)
#else
#define LOG(x, ...)                                        \
	do {                                               \
		Logger::getInstance().log(x, __VA_ARGS__); \
	} while (0)
#endif

#endif /* NEWSBOAT_LOGGER_H_ */
