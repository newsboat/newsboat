#include "3rd-party/catch.hpp"

#include <FilterParser.h>

TEST_CASE("FilterParser doesn't crash parsing expression with invalid character "
          "in operator", "[FilterParser]") {
	FilterParser fp;

	REQUIRE_FALSE(fp.parse_string("title =¯ \"foo\""));
}

TEST_CASE("FilterParser raises errors on invalid queries", "[FilterParser]") {
	FilterParser fp;

	SECTION("incorrect string quoting") {
		REQUIRE_FALSE(fp.parse_string("a = \"b"));
	}

	SECTION("non-value to the right of equality check operator") {
		REQUIRE_FALSE(fp.parse_string("a = b"));
	}

	SECTION("unbalanced parentheses") {
		REQUIRE_FALSE(fp.parse_string("((a=\"b\")))"));
	}

	SECTION("non-existent operator") {
		REQUIRE_FALSE(fp.parse_string("a !! \"b\""));
	}
}

TEST_CASE("FilterParser doesn't raise errors on valid queries",
          "[FilterParser]")
{
	FilterParser fp;

	SECTION("test parser") {
		REQUIRE(fp.parse_string("a = \"b\""));
		REQUIRE(fp.parse_string("(a=\"b\")"));
		REQUIRE(fp.parse_string("((a=\"b\"))"));
	}

	SECTION("test operators") {
		REQUIRE(fp.parse_string("a != \"b\""));
		REQUIRE(fp.parse_string("a =~ \"b\""));
		REQUIRE(fp.parse_string("a !~ \"b\""));
	}

	SECTION("Parse string of complex query") {
		REQUIRE(fp.parse_string("( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) and ( c !~ \"asdf\" )) or c != \"xx\""));
	}
}
