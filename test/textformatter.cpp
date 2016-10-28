#include "catch.hpp"

#include <textformatter.h>

using namespace newsbeuter;

TEST_CASE("textformatter: lines marked as `wrappable` are wrapped to fit width") {
	textformatter fmt;

	fmt.add_lines(
		{
			std::make_pair(wrappable, "this one is going to be wrapped"),
			std::make_pair(softwrappable, "this one is going to be wrapped at the window border"),
			std::make_pair(nonwrappable, "this one is going to be preserved even though it's much longer")
		});

	SECTION("formatting to plain text") {
		const std::string expected =
			"this one \n"
			"is going \n"
			"to be \n"
			"wrapped\n"
			"this one is going to be wrapped at the \n"
			"window border\n"
			"this one is going to be preserved even though it's much longer\n";
		REQUIRE(fmt.format_text_plain(10, 40) == expected);
	}

	SECTION("formatting to list") {
		const std::string expected =
			"{list"
				"{listitem text:\"this one \"}"
				"{listitem text:\"is going \"}"
				"{listitem text:\"to be \"}"
				"{listitem text:\"wrapped\"}"
				"{listitem text:\"this one is going to be wrapped at the \"}"
				"{listitem text:\"window border\"}"
				"{listitem text:\"this one is going to be preserved even though it's much longer\"}"
			"}";
		REQUIRE(fmt.format_text_to_list(nullptr, "", 10, 40) == expected);
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
	fmt.add_line(wrappable, "0123456789101112");
	fmt.add_line(softwrappable, "0123456789101112");
	fmt.add_line(nonwrappable, "0123456789101112");

	const std::string expected =
		"01234\n"
		"56789\n"
		"10111\n"
		"2\n"
		"0123456789\n"
		"101112\n"
		"0123456789101112\n";
	REQUIRE(fmt.format_text_plain(5, 10) == expected);
}

/*
 * A simple wrapping function would simply split the line at the wrapping width.
 * This, while it technically works, misaligns the text if the line is split
 * before the whitespace separating the words.
 * For example:
 * "just a test"
 * would be wrapped as
 * "just"
 * " a "
 * "test"
 * on a screen 4 columns wide. In this example, the 'a' is misaligned on the
 * second line and it would make the text look jagged with a bigger input. Thus
 * spaces at the beginning of lines after wrapping should be dropped.
 */
TEST_CASE("textformatter: ignore whitespace that's going to be wrapped onto the next line") {
	textformatter fmt;
	fmt.add_line(wrappable, "just a test");

	const std::string expected =
		"just\n"
		"a \n"
		"test\n";
	REQUIRE(fmt.format_text_plain(4) == expected);
}

TEST_CASE("textformatter: softwrappable lines are wrapped by format_text_to_list if total_width != 0") {
	textformatter fmt;
	fmt.add_line(softwrappable, "just a test");
	const size_t wrap_width = 100;
	regexmanager * rxman = nullptr;
	const std::string location = "";

	SECTION("total_width == 4") {
		const std::string expected =
			"{list"
				"{listitem text:\"just\"}"
				"{listitem text:\"a \"}"
				"{listitem text:\"test\"}"
			"}";
		REQUIRE(fmt.format_text_to_list(rxman, location, wrap_width, 4) == expected);
	}

	SECTION("total_width == 0") {
		const std::string expected =
			"{list"
				"{listitem text:\"just a test\"}"
			"}";
		REQUIRE(fmt.format_text_to_list(rxman, location, wrap_width, 0) == expected);
	}
}
