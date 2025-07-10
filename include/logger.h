#ifndef NEWSBOAT_LOGGER_H_
#define NEWSBOAT_LOGGER_H_

#include "strprintf.h"

#include "libNewsboat-ffi/src/logger.rs.h" // IWYU pragma: export

namespace Newsboat {

using Level = logger::Level;

namespace logger {

template<typename... Args>
void log(Level l, const std::string& format, Args... args)
{
	if (l == Level::USERERROR || static_cast<int64_t>(l) <= get_loglevel()) {
		log_internal(l, strprintf::fmt(format, args...));
	}
}
};

} // namespace Newsboat

// see https://kernelnewbies.org/FAQ/DoWhile0
#ifdef NDEBUG
#define LOG(x, ...) \
	do {        \
	} while (0)
#else
#define LOG(x, ...)                                        \
	do {                                               \
		Newsboat::logger::log(x, __VA_ARGS__); \
	} while (0)
#endif

#endif /* NEWSBOAT_LOGGER_H_ */
