#ifndef NEWSBOAT_LOGGER_H_
#define NEWSBOAT_LOGGER_H_

#include "config.h"
#include "strprintf.h"

namespace newsboat {
	enum class Level;
}

extern "C" {
	uint64_t rs_get_loglevel();
	void rs_log(newsboat::Level level, const char* message);
}

namespace newsboat {

// This has to be in sync with logger::Level in rust/libnewsboat/src/logger.rs
enum class Level { NONE = 0, USERERROR, CRITICAL, ERROR, WARN, INFO, DEBUG };

namespace Logger {
	void set_logfile(const std::string& logfile);
	void set_user_error_logfile(const std::string& logfile);
	void set_loglevel(Level l);

	template<typename... Args>
	void log(Level l, const std::string& format, Args... args)
	{
		if (l == Level::USERERROR
				|| static_cast<uint64_t>(l) <= rs_get_loglevel())
		{
			rs_log(l, strprintf::fmt(format, args...).c_str());
		}
	}
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
		newsboat::Logger::log(x, __VA_ARGS__); \
	} while (0)
#endif

#endif /* NEWSBOAT_LOGGER_H_ */
