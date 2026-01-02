#include "listformatter.h"

#include "3rd-party/catch.hpp"

using namespace newsboat;

namespace {

void add_lines(ListFormatter& listfmt, std::vector<std::string> lines)
{
	for (const auto& line : lines) {
		listfmt.add_line(StflRichText::from_plaintext(line));
	}
}

}

TEST_CASE("add_line(), get_lines_count() and clear()",
	"[ListFormatter]")
{
	ListFormatter fmt;

	REQUIRE(fmt.get_lines_count() == 0);

	SECTION("add_line() adds a line") {
		add_lines(fmt, {"one"});
		REQUIRE(fmt.get_lines_count() == 1);

		add_lines(fmt, {"two"});
		REQUIRE(fmt.get_lines_count() == 2);

		SECTION("clear() removes all lines") {
			fmt.clear();
			REQUIRE(fmt.get_lines_count() == 0);
		}
	}
}

TEST_CASE("set_line() replaces the item in a list", "[ListFormatter]")
{
	ListFormatter fmt;

	add_lines(fmt, {
		"hello",
		"goodb",
		"ye",
	});

	std::string expected =
		"{list"
		"{listitem text:\"hello\"}"
		"{listitem text:\"goodb\"}"
		"{listitem text:\"ye\"}"
		"}";
	REQUIRE(fmt.format_list() == expected);

	fmt.set_line(1, StflRichText::from_plaintext("oh"));

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
	ListFormatter fmt(&rxmgr, Dialog::Article);

	add_lines(fmt, {
		"Highlight me please!",
	});

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
