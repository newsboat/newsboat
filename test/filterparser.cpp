#include "FilterParser.h"

#include "3rd-party/catch.hpp"

TEST_CASE(
	"FilterParser doesn't crash parsing expression with invalid character "
	"in operator",
	"[FilterParser]")
{
	FilterParser fp;

	REQUIRE_FALSE(fp.parse_string("title =¯ \"foo\""));
}

TEST_CASE("FilterParser raises errors on invalid queries", "[FilterParser]")
{
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
		REQUIRE(fp.parse_string(
				"( a = \"b\") and ( b = \"c\" ) or ( ( c != \"d\" ) "
				"and ( c !~ \"asdf\" )) or c != \"xx\""));
	}
}

TEST_CASE("Both = and == are accepted", "[FilterParser]")
{
	FilterParser fp;

	REQUIRE(fp.parse_string("a = \"abc\""));
	REQUIRE(fp.parse_string("a == \"abc\""));
}

TEST_CASE("FilterParser disallows NUL byte inside filter expressions",
	"[FilterParser]")
{
	FilterParser fp;

	REQUIRE_FALSE(fp.parse_string(std::string("attri\0bute = 0", 14)));
	REQUIRE_FALSE(fp.parse_string(std::string("attribute\0= 0", 13)));
	REQUIRE_FALSE(fp.parse_string(std::string("attribute = \0", 13)));
	REQUIRE_FALSE(fp.parse_string(std::string("attribute = \\\"\0\\\"", 17)));

	// The following shouldn't pass, but it does. Further REQUIREs explain why:
	// the NUL byte silently terminates parsing.
	REQUIRE(fp.parse_string(std::string("attribute = \"hello\0world\"", 25)));
	REQUIRE(fp.get_root()->op == MATCHOP_EQ);
	REQUIRE(fp.get_root()->name == "attribute");
	REQUIRE(fp.get_root()->literal == "\"hello");
}

TEST_CASE("FilterParser parses empty string literals", "[FilterParser]")
{
	FilterParser fp;

	REQUIRE(fp.parse_string("title==\"\""));
	REQUIRE(fp.get_root()->op == MATCHOP_EQ);
	REQUIRE(fp.get_root()->name == "title");
	REQUIRE(fp.get_root()->literal == "");
}
