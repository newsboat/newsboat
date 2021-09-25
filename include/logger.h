#ifndef NEWSBOAT_LOGGER_H_
#define NEWSBOAT_LOGGER_H_

#include "config.h"
#include "strprintf.h"

#include "libnewsboat-ffi/src/logger.rs.h"

namespace newsboat {

using Level = Logger::Level;

namespace Logger {

template<typename... Args>
void log(Level l, const std::string& format, Args... args)
{
	if (l == Level::USERERROR || static_cast<int64_t>(l) <= get_loglevel()) {
		log_internal(l, strprintf::fmt(format, args...));
	}
}
};

} // namespace newsboat

// see https://kernelnewbies.org/FAQ/DoWhile0
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
