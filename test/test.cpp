#define CATCH_CONFIG_RUNNER
#include "3rd-party/catch.hpp"

#include "logger.h"

int main(int argc, char *argv[])
{
	setlocale(LC_CTYPE, "");

	srand(static_cast<unsigned int>(std::time(0)));

	return Catch::Session().run(argc, argv);
}

