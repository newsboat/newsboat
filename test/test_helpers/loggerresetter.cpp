#include "loggerresetter.h"

#include "logger.h"

namespace test_helpers {

void reset_logger()
{
	::Newsboat::logger::set_logfile("/dev/null");
	::Newsboat::logger::set_user_error_logfile("/dev/null");
	::Newsboat::logger::unset_loglevel();
}

LoggerResetter::LoggerResetter()
{
	reset_logger();
}

LoggerResetter::~LoggerResetter()
{
	reset_logger();
}

} /* namespace test_helpers */
