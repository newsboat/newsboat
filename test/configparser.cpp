#include "catch.hpp"

#include <configparser.h>

using namespace newsbeuter;

TEST_CASE("evaluate_backticks replaces command in backticks with its output",
          "[configparser]") {
	SECTION("substitutes command output") {
		REQUIRE(configparser::evaluate_backticks("") == "");
		REQUIRE(configparser::evaluate_backticks("hello world") == "hello world");
		// backtick evaluation with true (empty string)
		REQUIRE(configparser::evaluate_backticks("foo`true`baz") == "foobaz");
		// backtick evaluation with missing second backtick
		REQUIRE(configparser::evaluate_backticks("foo`barbaz") == "foo`barbaz");
		// backtick evaluation with true (2)
		REQUIRE(configparser::evaluate_backticks("foo `true` baz") == "foo  baz");
		// backtick evaluation with missing third backtick
		REQUIRE(configparser::evaluate_backticks("foo `true` baz `xxx")
				== "foo  baz `xxx");
		// backtick evaluation of an echo command
		REQUIRE(configparser::evaluate_backticks("`echo hello world`")
				== "hello world");
		// backtick evaluation of an echo command embedded in regular text
		REQUIRE(configparser::evaluate_backticks("xxx`echo yyy`zzz")
				== "xxxyyyzzz");
		// backtick evaluation of an expression piped into bc
		REQUIRE(configparser::evaluate_backticks("`echo 3 \\* 4 | bc`") == "12");
	}

	SECTION("backticks can be escaped with backslash") {
		REQUIRE(configparser::evaluate_backticks("hehe \\`two at a time\\`haha")
				== "hehe `two at a time`haha");
	}

	SECTION("single backticks have to be escaped too") {
		REQUIRE(configparser::evaluate_backticks("a single literal backtick: \\`")
				== "a single literal backtick: `");
	}
}
