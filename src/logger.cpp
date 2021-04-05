#include "logger.h"

namespace newsboat {
void Logger::set_logfile(const std::string& logfile)
{
	logger::bridged::set_logfile(logfile);
}

void Logger::set_user_error_logfile(const std::string& logfile)
{
	logger::bridged::set_user_error_logfile(logfile);
}

void Logger::set_loglevel(Level l)
{
	logger::bridged::set_loglevel(l);
}

void Logger::unset_loglevel()
{
	logger::bridged::unset_loglevel();
}

} // namespace newsboat
