#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <logger.h>

int main(int argc, char *argv[])
{
	setlocale(LC_CTYPE, "");

	return Catch::Session().run(argc, argv);
}

