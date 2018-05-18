#include "listformatter.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

TEST_CASE("add_line(), add_lines(), get_lines_count() and clear()",
	"[listformatter]")
{
	listformatter fmt;

	REQUIRE(fmt.get_lines_count() == 0);

	SECTION("add_line() adds a line")
	{
		fmt.add_line("one");
		REQUIRE(fmt.get_lines_count() == 1);

		fmt.add_line("two");
		REQUIRE(fmt.get_lines_count() == 2);

		SECTION("add_lines() adds multiple lines")
		{
			fmt.add_lines({"three", "four"});
			REQUIRE(fmt.get_lines_count() == 4);

			SECTION("clear() removes all lines")
			{
				fmt.clear();
				REQUIRE(fmt.get_lines_count() == 0);
			}
		}
	}
}

TEST_CASE("add_line() splits overly long sequences to fit width",
	"[listformatter]")
{
	listformatter fmt;

	SECTION("ordinary text")
	{
		fmt.add_line("123456789_", UINT_MAX, 10);
		fmt.add_line("_987654321", UINT_MAX, 10);
		fmt.add_line("listformatter doesn't care about word boundaries",
			UINT_MAX,
			10);
		std::string expected =
			"{list"
			"{listitem text:\"123456789_\"}"
			"{listitem text:\"_987654321\"}"
			"{listitem text:\"listformat\"}"
			"{listitem text:\"ter doesn'\"}"
			"{listitem text:\"t care abo\"}"
			"{listitem text:\"ut word bo\"}"
			"{listitem text:\"undaries\"}"
			"}";
		REQUIRE(fmt.format_list(nullptr, "") == expected);
	}

	SECTION("numbered list")
	{
		fmt.add_line("123456789_", 1, 10);
		fmt.add_line("_987654321", 2, 10);
		fmt.add_line("listformatter doesn't care about word boundaries",
			3,
			10);
		std::string expected =
			"{list"
			"{listitem[1] text:\"123456789_\"}"
			"{listitem[2] text:\"_987654321\"}"
			"{listitem[3] text:\"listformat\"}"
			"{listitem[3] text:\"ter doesn'\"}"
			"{listitem[3] text:\"t care abo\"}"
			"{listitem[3] text:\"ut word bo\"}"
			"{listitem[3] text:\"undaries\"}"
			"}";
		REQUIRE(fmt.format_list(nullptr, "") == expected);
	}
}

TEST_CASE("set_line() replaces the item in a list", "[listformatter]")
{
	listformatter fmt;

	fmt.add_line("hello", 1, 5);
	fmt.add_line("goodbye", 2, 5);

	std::string expected =
		"{list"
		"{listitem[1] text:\"hello\"}"
		"{listitem[2] text:\"goodb\"}"
		"{listitem[2] text:\"ye\"}"
		"}";
	REQUIRE(fmt.format_list(nullptr, "") == expected);

	fmt.set_line(1, "oh", 3, 3);

	expected =
		"{list"
		"{listitem[1] text:\"hello\"}"
		"{listitem[3] text:\"oh\"}"
		"{listitem[2] text:\"ye\"}"
		"}";
	REQUIRE(fmt.format_list(nullptr, "") == expected);
}

TEST_CASE("format_list() uses regex manager if one is passed",
	"[listformatter]")
{
	listformatter fmt;

	fmt.add_line("Highlight me please!");

	regexmanager rxmgr;
	// the choice of green text on red background does not reflect my
	// personal taste (or lack thereof) :)
	rxmgr.handle_action(
		"highlight", {"article", "please", "green", "default"});

	std::string expected =
		"{list"
		"{listitem text:\"Highlight me <0>please</>!\"}"
		"}";

	REQUIRE(fmt.format_list(&rxmgr, "article") == expected);
}
