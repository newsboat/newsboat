#define CATCH_CONFIG_RUNNER
#include "3rd-party/catch.hpp"

#include "test_helpers/httptestserver.h"
#include "tui.h"

int main(int argc, char* argv[])
{
	setlocale(LC_CTYPE, "");

	srand(static_cast<unsigned int>(std::time(0)));

	test_helpers::HttpTestServer server;

	// TODO: Remove
	newsboat::Tui tui;

	return Catch::Session().run(argc, argv);
}
