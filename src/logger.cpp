#include "logger.h"

extern "C" {
	void rs_set_logfile(const char* logfile);
	void rs_set_user_error_logfile(const char* logfile);
	void rs_set_loglevel(newsboat::Level level);
	void rs_unset_loglevel();
}

namespace newsboat {
void Logger::set_logfile(const std::string& logfile)
{
	rs_set_logfile(logfile.c_str());
}

void Logger::set_user_error_logfile(const std::string& logfile)
{
	rs_set_user_error_logfile(logfile.c_str());
}

void Logger::set_loglevel(Level l)
{
	rs_set_loglevel(l);
}

void Logger::unset_loglevel()
{
	rs_unset_loglevel();
}

} // namespace newsboat
