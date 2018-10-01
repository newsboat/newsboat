#include "configparser.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("evaluate_backticks replaces command in backticks with its output",
	"[ConfigParser]")
{
	SECTION("substitutes command output")
	{
		REQUIRE(ConfigParser::evaluate_backticks("") == "");
		REQUIRE(ConfigParser::evaluate_backticks("hello world") ==
			"hello world");
		// backtick evaluation with true (empty string)
		REQUIRE(ConfigParser::evaluate_backticks("foo`true`baz") ==
			"foobaz");
		// backtick evaluation with true (2)
		REQUIRE(ConfigParser::evaluate_backticks("foo `true` baz") ==
			"foo  baz");
		REQUIRE(ConfigParser::evaluate_backticks("foo`barbaz") ==
			"foo`barbaz");
		REQUIRE(ConfigParser::evaluate_backticks(
				"foo `true` baz `xxx") == "foo  baz `xxx");
		REQUIRE(ConfigParser::evaluate_backticks(
				"`echo hello world`") == "hello world");
		REQUIRE(ConfigParser::evaluate_backticks("xxx`echo yyy`zzz") ==
			"xxxyyyzzz");
		REQUIRE(ConfigParser::evaluate_backticks(
				"`seq 10 | tail -1`") == "10");
	}

	SECTION("backticks can be escaped with backslash")
	{
		REQUIRE(ConfigParser::evaluate_backticks(
				"hehe \\`two at a time\\`haha") ==
			"hehe `two at a time`haha");
	}

	SECTION("single backticks have to be escaped too")
	{
		REQUIRE(ConfigParser::evaluate_backticks(
				"a single literal backtick: \\`") ==
			"a single literal backtick: `");
	}
}
