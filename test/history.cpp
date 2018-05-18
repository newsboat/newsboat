#include "history.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("History can be iterated on in any direction", "[history]")
{
	history h;

	SECTION("Empty history returns nothing")
	{
		REQUIRE(h.prev() == "");
		REQUIRE(h.prev() == "");
		REQUIRE(h.next() == "");
		REQUIRE(h.next() == "");

		SECTION("One line in history")
		{
			h.add_line("testline");
			REQUIRE(h.prev() == "testline");
			REQUIRE(h.prev() == "testline");
			REQUIRE(h.next() == "testline");
			REQUIRE(h.next() == "");

			SECTION("Two lines in history")
			{
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
