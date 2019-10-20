#include "history.h"

#include "3rd-party/catch.hpp"
#include "test-helpers.h"

using namespace newsboat;

TEST_CASE("History can be iterated on in any direction", "[History]")
{
	History h;

	SECTION("Empty History returns nothing") {
		REQUIRE(h.prev() == "");
		REQUIRE(h.prev() == "");
		REQUIRE(h.next() == "");
		REQUIRE(h.next() == "");

		SECTION("One line in History") {
			h.add_line("testline");
			REQUIRE(h.prev() == "testline");
			REQUIRE(h.prev() == "testline");
			REQUIRE(h.next() == "testline");
			REQUIRE(h.next() == "");

			SECTION("Two lines in History") {
				h.add_line("foobar");
				REQUIRE(h.prev() == "foobar");
				REQUIRE(h.prev() == "testline");
				REQUIRE(h.next() == "testline");
				REQUIRE(h.prev() == "testline");
				REQUIRE(h.next() == "testline");
				REQUIRE(h.next() == "foobar");
				REQUIRE(h.next() == "");
				REQUIRE(h.next() == "");
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
			REQUIRE(loaded_h.prev() == "foobar");
			REQUIRE(loaded_h.prev() == "testline");
			REQUIRE(loaded_h.next() == "testline");
			REQUIRE(loaded_h.next() == "foobar");
			REQUIRE(loaded_h.next() == "");
		}
	}
}
