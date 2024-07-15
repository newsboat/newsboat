#include "scopemeasure.h"

#include "3rd-party/catch.hpp"

#include <fstream>

#include "logger.h"

#include "test_helpers/loggerresetter.h"
#include "test_helpers/tempfile.h"

using namespace newsboat;

unsigned int file_lines_count(const Filepath& filepath)
{
	unsigned int line_count = 0;
	std::ifstream in(filepath.to_locale_string());
	std::string line;
	while (std::getline(in, line)) {
		line_count++;
	}

	return line_count;
}

TEST_CASE("Destroying a ScopeMeasure object writes a line to the log",
	"[ScopeMeasure]")
{
	test_helpers::TempFile tmp;

	{
		test_helpers::LoggerResetter logReset;
		const auto filepath = tmp.get_path();
		logger::set_logfile(filepath);
		logger::set_loglevel(Level::DEBUG);

		ScopeMeasure sm("test");
	}

	REQUIRE(file_lines_count(tmp.get_path()) == 1);
}

TEST_CASE("stopover() adds an extra line to the log upon each call",
	"[ScopeMeasure]")
{
	test_helpers::TempFile tmp;

	// initialized to an impossible value to catch logical errors in the test
	// itself
	unsigned int expected_line_count = 100500;

	{
		test_helpers::LoggerResetter logReset;
		const auto filepath = tmp.get_path();
		logger::set_logfile(filepath);
		logger::set_loglevel(Level::DEBUG);

		ScopeMeasure sm("test");

		SECTION("one call") {
			sm.stopover("here");
			// One line from stopover(), one line from destructor
			expected_line_count = 2;
		}

		SECTION("two calls") {
			sm.stopover("here");
			sm.stopover("there");
			// Two lines from stopover(), one line from destructor
			expected_line_count = 3;
		}

		SECTION("five calls") {
			sm.stopover("here");
			sm.stopover("there");
			sm.stopover("and");
			sm.stopover("also");
			sm.stopover("here");
			// Five lines from stopover(), one line from destructor
			expected_line_count = 6;
		}
	}

	REQUIRE(file_lines_count(tmp.get_path()) == expected_line_count);
}
