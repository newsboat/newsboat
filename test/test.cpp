#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <logger.h>

int main(int argc, char *argv[])
{
	setlocale(LC_CTYPE, "");
	newsbeuter::logger::getInstance().set_logfile("testlog.txt");
	newsbeuter::logger::getInstance().set_loglevel(newsbeuter::LOG_DEBUG);

	return Catch::Session().run(argc, argv);
}

