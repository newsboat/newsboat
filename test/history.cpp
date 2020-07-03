#include "history.h"

#include <unistd.h>

#include "3rd-party/catch.hpp"
#include "test-helpers/tempdir.h"

using namespace newsboat;

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
	TestHelpers::TempDir tmp;
	const auto filepath = tmp.get_path() + "history.cmdline";

	History h;
	h.add_line("testline");
	h.add_line("foobar");

	SECTION("Nothing is saved to file if limit is zero") {
		h.save_to_file(filepath, 0);
		REQUIRE_FALSE(0 == ::access(filepath.c_str(), R_OK | W_OK));
	}

	SECTION("Save to file") {
		h.save_to_file(filepath, 10);
		REQUIRE(0 == ::access(filepath.c_str(), R_OK | W_OK));

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
