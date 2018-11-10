#include "logger.h"

#ifdef __cplusplus
extern "C" {
#endif

void rs_set_logfile(const char* logfile);
void rs_set_errorlogfile(const char* logfile);
void rs_set_loglevel(newsboat::Level level);

#ifdef __cplusplus
}
#endif

namespace newsboat {
void Logger::set_logfile(const std::string& logfile)
{
	rs_set_logfile(logfile.c_str());
}

void Logger::set_errorlogfile(const std::string& logfile)
{
	rs_set_errorlogfile(logfile.c_str());
}

void Logger::set_loglevel(Level l)
{
	rs_set_loglevel(l);
}

} // namespace newsboat
