#include "catch.hpp"

#include <textformatter.h>

using namespace newsbeuter;

TEST_CASE("textformatter: lines marked as `wrappable` are wrapped to fit width") {
	textformatter fmt;

	fmt.add_lines(
		{
			std::make_pair(wrappable, "this one is going to be wrapped"),
			std::make_pair(nonwrappable, "this one is going to be preserved")
		});

	SECTION("formatting to plain text") {
		const std::string expected =
			"this one \n"
			"is going \n"
			"to be \n"
			"wrapped\n"
			"this one is going to be preserved\n";
		REQUIRE(fmt.format_text_plain(10) == expected);
	}

	SECTION("formatting to list") {
		const std::string expected =
			"{list"
				"{listitem text:\"this one \"}"
				"{listitem text:\"is going \"}"
				"{listitem text:\"to be \"}"
				"{listitem text:\"wrapped\"}"
				"{listitem text:\"this one is going to be preserved\"}"
			"}";
		REQUIRE(fmt.format_text_to_list(NULL, "", 10) == expected);
	}
}

TEST_CASE("textformatter: regex manager is used by format_text_to_list if one is passed") {
	textformatter fmt;

	fmt.add_line(wrappable, "Highlight me please!");

	regexmanager rxmgr;
	// the choice of green text on red background does not reflect my personal
	// taste (or lack thereof) :)
	rxmgr.handle_action("highlight", {"article", "please", "green", "default"});

	const std::string expected =
		"{list"
			"{listitem text:\"Highlight me <0>please</>!\"}"
		"}";

	REQUIRE(fmt.format_text_to_list(&rxmgr, "article", 100) == expected);
}

TEST_CASE("textformatter: <hr> is rendered properly") {
	textformatter fmt;

	fmt.add_line(hr, "");

	SECTION("width = 10") {
		const std::string expected =
			"\n"
			" -------- "
			"\n"
			"\n";
		REQUIRE(fmt.format_text_plain(10) == expected);
	}
}

TEST_CASE("textformatter: wrappable sequences longer then format width are forced-wrapped") {
	textformatter fmt;
	fmt.add_line(wrappable, "0123456789");
	fmt.add_line(nonwrappable, "0123456789");

	const std::string expected =
		"01234\n"
		"56789\n"
		"0123456789\n";
	REQUIRE(fmt.format_text_plain(5) == expected);
}

TEST_CASE("textformatter: when wrapping, spaces at the beginning of lines are dropped") {
	textformatter fmt;
	fmt.add_line(wrappable, "just a test");

	const std::string expected =
		"just\n"
		"a \n"
		"test\n";
	REQUIRE(fmt.format_text_plain(4) == expected);
}
