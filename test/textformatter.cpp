#include "textformatter.h"

#include "3rd-party/catch.hpp"

#include "regexmanager.h"

using namespace newsboat;

TEST_CASE("lines marked as `wrappable` are wrapped to fit width",
	"[TextFormatter]")
{
	TextFormatter fmt({std::make_pair(LineType::wrappable,
				"this one is going to be wrapped"),
			std::make_pair(LineType::softwrappable,
				"this one is going to be wrapped at the window "
				"border"),
			std::make_pair(LineType::nonwrappable,
				"this one is going to be preserved even though "
				"it's much longer")});

	SECTION("formatting to plain text") {
		const std::string expected =
			"this one \n"
			"is going \n"
			"to be \n"
			"wrapped\n"
			"this one is going to be wrapped at the \n"
			"window border\n"
			"this one is going to be preserved even though it's "
			"much longer\n";
		REQUIRE(fmt.format_text_plain(10, 40) == expected);
	}

	SECTION("formatting to list") {
		const std::string expected_text =
			"{list"
			"{listitem text:\"this one \"}"
			"{listitem text:\"is going \"}"
			"{listitem text:\"to be \"}"
			"{listitem text:\"wrapped\"}"
			"{listitem text:\"this one is going to be wrapped at "
			"the \"}"
			"{listitem text:\"window border\"}"
			"{listitem text:\"this one is going to be preserved "
			"even though it's much longer\"}"
			"}";
		const std::size_t expected_count = 7;

		const auto result =
			fmt.format_text_to_list(nullptr, std::nullopt, 10, 40);

		REQUIRE(result.first == expected_text);
		REQUIRE(result.second == expected_count);
	}
}

TEST_CASE("line wrapping works for non-space-separated text", "[TextFormatter]")
{
	TextFormatter fmt({std::make_pair(LineType::wrappable,
				"    つれづれなるままに、ひぐらしすずりにむかいて、")});

	SECTION("preserve indent and doesn't return broken UTF-8") {
		const std::string expected =
			("    つれづれなるまま\n"
				"    に、ひぐらしすず\n"
				"    りにむかいて、\n");
		REQUIRE(fmt.format_text_plain(20) == expected);
		// +1 is not enough to store single wide-width char
		REQUIRE(fmt.format_text_plain(20 + 1) == expected);
	}

	SECTION("truncate indent if given window width is too narrow") {
		REQUIRE(fmt.format_text_plain(1) == (" \n"));
		REQUIRE(fmt.format_text_plain(2) == ("  \n"));
		REQUIRE(fmt.format_text_plain(3) == ("   \n"));
	}

	SECTION("discard current word if there's not enough space to put "
		"single char") {
		REQUIRE(fmt.format_text_plain(4) == ("    \n"));
		REQUIRE(fmt.format_text_plain(5) == ("    \n"));
	}
}

TEST_CASE("regex manager is used by format_text_to_list if one is passed",
	"[TextFormatter]")
{
	TextFormatter fmt({{LineType::wrappable, "Highlight me please!"}});

	RegexManager rxmgr;
	// the choice of green text on red background does not reflect my
	// personal taste (or lack thereof) :)
	rxmgr.handle_action(
		"highlight", {"article", "please", "green", "default"});

	const std::string expected_text =
		"{list"
		"{listitem text:\"Highlight me <0>please</>!\"}"
		"}";
	const std::size_t expected_count = 1;

	const auto result = fmt.format_text_to_list(&rxmgr, Dialog::Article, 100);

	REQUIRE(result.first == expected_text);
	REQUIRE(result.second == expected_count);
}

TEST_CASE("<hr> is rendered as a string of dashes framed with newlines",
	"[TextFormatter]")
{
	TextFormatter fmt({{LineType::hr, ""}});

	SECTION("width = 3") {
		const std::string expected =
			"\n"
			" - "
			"\n"
			"\n";
		REQUIRE(fmt.format_text_plain(3) == expected);
	}

	SECTION("width = 10") {
		const std::string expected =
			"\n"
			" -------- "
			"\n"
			"\n";
		REQUIRE(fmt.format_text_plain(10) == expected);
	}

	SECTION("width = 72") {
		const std::string expected =
			"\n"
			" -----------------------------------------------------"
			"----------------- "
			"\n"
			"\n";
		REQUIRE(fmt.format_text_plain(72) == expected);
	}
}

TEST_CASE("wrappable sequences longer then format width are forced-wrapped",
	"[TextFormatter]")
{
	TextFormatter fmt({{LineType::wrappable, "0123456789101112"},
		{LineType::softwrappable, "0123456789101112"},
		{LineType::nonwrappable, "0123456789101112"}});

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

TEST_CASE("Lines marked as non-wrappable are always returned verbatim",
	"[TextFormatter]")
{
	TextFormatter fmt({{LineType::wrappable, " 0123456789101112"},
		{LineType::softwrappable, " 0123456789101112"},
		{LineType::nonwrappable, " 0123456789101112"}});

	const std::string expected =
		" \n"
		" \n"
		" 0123456789101112\n";
	REQUIRE(fmt.format_text_plain(1, 1) == expected);
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
TEST_CASE("Ignore whitespace that's going to be wrapped onto the next line",
	"[TextFormatter]")
{
	TextFormatter fmt({{LineType::wrappable, "just a test"}});

	const std::string expected =
		"just\n"
		"a \n"
		"test\n";
	REQUIRE(fmt.format_text_plain(4) == expected);
}

TEST_CASE(
	"softwrappable lines are wrapped by format_text_to_list if "
	"total_width != 0",
	"[TextFormatter]")
{
	TextFormatter fmt({{LineType::softwrappable, "just a test"}});
	const size_t wrap_width = 100;
	RegexManager* rxman = nullptr;

	SECTION("total_width == 4") {
		const std::string expected_text =
			"{list"
			"{listitem text:\"just\"}"
			"{listitem text:\"a \"}"
			"{listitem text:\"test\"}"
			"}";
		const std::size_t expected_count = 3;

		const auto result =
			fmt.format_text_to_list(rxman, std::nullopt, wrap_width, 4);

		REQUIRE(result.first == expected_text);
		REQUIRE(result.second == expected_count);
	}

	SECTION("total_width == 0") {
		const std::string expected_text =
			"{list"
			"{listitem text:\"just a test\"}"
			"}";
		const std::size_t expected_count = 1;

		const auto result =
			fmt.format_text_to_list(rxman, std::nullopt, wrap_width, 0);

		REQUIRE(result.first == expected_text);
		REQUIRE(result.second == expected_count);
	}
}

TEST_CASE("Lines consisting entirely of spaces are replaced "
	"by a single space",
	"[TextFormatter]")
{
	// Limit for wrappable lines
	const size_t wrap_width = 3;
	// Limit for softwrapable lines
	const size_t total_width = 4;

	// All of these lines are longer than the wrap limits. If these lines were
	// wrapped, the result would have more than 4 lines.
	TextFormatter fmt({{LineType::wrappable, "    "},
		{LineType::wrappable, "          "},
		{LineType::softwrappable, "      "},
		{LineType::softwrappable, "  "}});

	const std::string expected_text =
		"{list"
		"{listitem text:\" \"}"
		"{listitem text:\" \"}"
		"{listitem text:\" \"}"
		"{listitem text:\" \"}"
		"}";
	const std::size_t expected_count = 4;

	RegexManager* rxman = nullptr;
	const auto result = fmt.format_text_to_list(rxman, std::nullopt, wrap_width,
			total_width);

	REQUIRE(result.first == expected_text);
	REQUIRE(result.second == expected_count);
}
