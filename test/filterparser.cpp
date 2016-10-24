#include "catch.hpp"

#include <FilterParser.h>

TEST_CASE("FilterParser doesn't crash parsing expression with invalid character "
          "in operator", "[FilterParser]") {
	FilterParser fp;

	REQUIRE_FALSE(fp.parse_string("title =¯ \"foo\""));
}

TEST_CASE("FilterParser behaves correctly", "[FilterParser]") {
	FilterParser fp;

	SECTION("test parser") {
		REQUIRE(fp.parse_string("a = \"b\""));
		REQUIRE_FALSE(fp.parse_string("a = \"b"));
		REQUIRE_FALSE(fp.parse_string("a = b"));
		REQUIRE(fp.parse_string("(a=\"b\")"));
		REQUIRE(fp.parse_string("((a=\"b\"))"));
		REQUIRE_FALSE(fp.parse_string("((a=\"b\")))"));
	}

	SECTION("test operators") {
		REQUIRE(fp.parse_string("a != \"b\""));
		REQUIRE(fp.parse_string("a =~ \"b\""));
		REQUIRE(fp.parse_string("a !~ \"b\""));
		REQUIRE_FALSE(fp.parse_string("a !! \"b\""));
	}

	// complex query
	SECTION("Parse string of complex query") {
		REQUIRE(fp.parse_string("( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) and ( c !~ \"asdf\" )) or c != \"xx\""));
	}
}
