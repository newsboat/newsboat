#include "listformatter.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("add_line(), add_lines(), get_lines_count() and clear()",
	"[ListFormatter]")
{
	ListFormatter fmt;

	REQUIRE(fmt.get_lines_count() == 0);

	SECTION("add_line() adds a line") {
		fmt.add_line("one");
		REQUIRE(fmt.get_lines_count() == 1);

		fmt.add_line("two");
		REQUIRE(fmt.get_lines_count() == 2);

		SECTION("add_lines() adds multiple lines") {
			fmt.add_lines({"three", "four"});
			REQUIRE(fmt.get_lines_count() == 4);

			SECTION("clear() removes all lines") {
				fmt.clear();
				REQUIRE(fmt.get_lines_count() == 0);
			}
		}
	}
}

TEST_CASE("add_line() splits overly long sequences to fit width",
	"[ListFormatter]")
{
	ListFormatter fmt;

	SECTION("ordinary text") {
		fmt.add_line("123456789_", 10);
		fmt.add_line("_987654321", 10);
		fmt.add_line("ListFormatter doesn't care about word boundaries",
			10);
		std::string expected =
			"{list"
			"{listitem text:\"123456789_\"}"
			"{listitem text:\"_987654321\"}"
			"{listitem text:\"ListFormat\"}"
			"{listitem text:\"ter doesn'\"}"
			"{listitem text:\"t care abo\"}"
			"{listitem text:\"ut word bo\"}"
			"{listitem text:\"undaries\"}"
			"}";
		REQUIRE(fmt.format_list() == expected);
	}

	SECTION("numbered list") {
		fmt.add_line("123456789_", 10);
		fmt.add_line("_987654321", 10);
		fmt.add_line("ListFormatter doesn't care about word boundaries",
			10);
		std::string expected =
			"{list"
			"{listitem text:\"123456789_\"}"
			"{listitem text:\"_987654321\"}"
			"{listitem text:\"ListFormat\"}"
			"{listitem text:\"ter doesn'\"}"
			"{listitem text:\"t care abo\"}"
			"{listitem text:\"ut word bo\"}"
			"{listitem text:\"undaries\"}"
			"}";
		REQUIRE(fmt.format_list() == expected);
	}
}

TEST_CASE("set_line() replaces the item in a list", "[ListFormatter]")
{
	ListFormatter fmt;

	fmt.add_line("hello", 5);
	fmt.add_line("goodbye", 5);

	std::string expected =
		"{list"
		"{listitem text:\"hello\"}"
		"{listitem text:\"goodb\"}"
		"{listitem text:\"ye\"}"
		"}";
	REQUIRE(fmt.format_list() == expected);

	fmt.set_line(1, "oh", 3);

	expected =
		"{list"
		"{listitem text:\"hello\"}"
		"{listitem text:\"oh\"}"
		"{listitem text:\"ye\"}"
		"}";
	REQUIRE(fmt.format_list() == expected);
}

TEST_CASE("format_list() uses regex manager if one is passed",
	"[ListFormatter]")
{
	RegexManager rxmgr;
	ListFormatter fmt(&rxmgr, "article");

	fmt.add_line("Highlight me please!");

	// the choice of green text on red background does not reflect my
	// personal taste (or lack thereof) :)
	rxmgr.handle_action(
		"highlight", {"article", "please", "green", "default"});

	std::string expected =
		"{list"
		"{listitem text:\"Highlight me <0>please</>!\"}"
		"}";

	REQUIRE(fmt.format_list() == expected);
}
