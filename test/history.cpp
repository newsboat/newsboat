#include "history.h"

#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "test_helpers/tempdir.h"

using namespace newsboat;

bool file_available_for_reading_and_writing(const Filepath& filepath)
{
	const auto filepath_str = filepath.to_locale_string();
	return (0 == ::access(filepath_str.c_str(), R_OK | W_OK));
}

TEST_CASE("History can be iterated on in any direction", "[History]")
{
	History h;

	SECTION("Empty History returns nothing") {
		REQUIRE(h.previous_line() == "");
		REQUIRE(h.previous_line() == "");
		REQUIRE(h.next_line() == "");
		REQUIRE(h.next_line() == "");

		SECTION("One line in History") {
			h.add_line("testline");
			REQUIRE(h.previous_line() == "testline");
			REQUIRE(h.previous_line() == "testline");
			REQUIRE(h.next_line() == "testline");
			REQUIRE(h.next_line() == "");

			SECTION("Two lines in History") {
				h.add_line("foobar");
				REQUIRE(h.previous_line() == "foobar");
				REQUIRE(h.previous_line() == "testline");
				REQUIRE(h.next_line() == "testline");
				REQUIRE(h.previous_line() == "testline");
				REQUIRE(h.next_line() == "testline");
				REQUIRE(h.next_line() == "foobar");
				REQUIRE(h.next_line() == "");
				REQUIRE(h.next_line() == "");
			}
		}
	}
}

TEST_CASE("History can be saved and loaded from file", "[History]")
{
	test_helpers::TempDir tmp;
	const auto filepath = tmp.get_path().join("history.cmdline"_path);

	History h;
	h.add_line("testline");
	h.add_line("foobar");

	SECTION("Nothing is saved to file if limit is zero") {
		h.save_to_file(filepath, 0);
		REQUIRE_FALSE(file_available_for_reading_and_writing(filepath));
	}

	SECTION("Save to file") {
		h.save_to_file(filepath, 10);
		REQUIRE(file_available_for_reading_and_writing(filepath));

		SECTION("Load from file") {
			History loaded_h;
			loaded_h.load_from_file(filepath);
			REQUIRE(loaded_h.previous_line() == "foobar");
			REQUIRE(loaded_h.previous_line() == "testline");
			REQUIRE(loaded_h.next_line() == "testline");
			REQUIRE(loaded_h.next_line() == "foobar");
			REQUIRE(loaded_h.next_line() == "");
		}
	}
}

TEST_CASE("Only the most recent lines are saved when limiting history",
	"[History]")
{
	test_helpers::TempDir tmp;
	const auto filepath = tmp.get_path().join("history.cmdline"_path);
	const int max_lines = 3;

	History h;
	h.add_line("1");
	h.add_line("2");
	h.add_line("3");
	h.add_line("4");

	SECTION("file with history lines is created") {
		h.save_to_file(filepath, max_lines);
		REQUIRE(file_available_for_reading_and_writing(filepath));

		SECTION("when loading, only a limited number of lines are returned") {
			History loaded_h;
			loaded_h.load_from_file(filepath);
			REQUIRE(loaded_h.previous_line() == "4");
			REQUIRE(loaded_h.previous_line() == "3");
			REQUIRE(loaded_h.previous_line() == "2");
			REQUIRE(loaded_h.previous_line() == "2");
		}
	}
}
