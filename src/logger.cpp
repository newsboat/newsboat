#include "logger.h"

#include <cerrno>
#include <stdarg.h>

#include "exception.h"

namespace newsboat {
std::mutex Logger::instanceMutex;

Logger::Logger()
	: curlevel(Level::NONE)
{
}

void Logger::set_logfile(const std::string& logfile)
{
	/*
	 * This sets the filename of the debug logfile
	 */
	std::lock_guard<std::mutex> lock(logMutex);
	if (f.is_open())
		f.close();
	f.open(logfile, std::fstream::out);
	if (!f.is_open()) {
		throw Exception(errno); // the question is whether f.open() sets
					// errno...
	}
}

void Logger::set_errorlogfile(const std::string& logfile)
{
	/*
	 * This sets the filename of the error logfile, i.e. the one that can be
	 * configured to be generated.
	 */
	std::lock_guard<std::mutex> lock(logMutex);
	if (ef.is_open())
		ef.close();
	ef.open(logfile, std::fstream::out);
	if (!ef.is_open()) {
		throw Exception(errno);
	}
	if (Level::NONE == curlevel) {
		curlevel = Level::USERERROR;
	}
}

void Logger::set_loglevel(Level l)
{
	std::lock_guard<std::mutex> lock(logMutex);
	curlevel = l;
	if (curlevel == Level::NONE)
		f.close();
}

Logger& Logger::getInstance()
{
	/*
	 * This is the global logger that everyone uses
	 */
	std::lock_guard<std::mutex> lock(instanceMutex);
	static Logger theLogger;
	return theLogger;
}

} // namespace newsboat
