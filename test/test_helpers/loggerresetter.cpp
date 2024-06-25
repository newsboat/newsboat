#define ENABLE_IMPLICIT_FILEPATH_CONVERSIONS

#include "loggerresetter.h"

#include "logger.h"

namespace test_helpers {

void reset_logger()
{
	const auto path = "/dev/null";
	const auto filepath = ::newsboat::Filepath::from_locale_string(path);
	::newsboat::logger::set_logfile(filepath);
	::newsboat::logger::set_user_error_logfile(filepath);
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
