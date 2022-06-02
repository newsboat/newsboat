#include "loggerresetter.h"

#include "logger.h"

namespace test_helpers {

void reset_logger()
{
	::newsboat::logger::set_logfile("/dev/null");
	::newsboat::logger::set_user_error_logfile("/dev/null");
	::newsboat::logger::unset_loglevel();
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
