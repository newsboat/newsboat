#include "stflrichtext.h"

#include "3rd-party/catch.hpp"

#include <map>
#include <string>
#include <vector>

using namespace Newsboat;

TEST_CASE("Constructing StflRichText from plaintext string does not make changes",
	"[StflRichText]")
{
	const std::vector<std::string> test_strings {
		"",
		"foo",
		"<tag>",
		">test",
		"<test",
		"test<",
		"test>",
	};

	for (const auto& input : test_strings) {
		DYNAMIC_SECTION("plaintext: " << input) {
			const auto stflString = StflRichText::from_plaintext(input);
			REQUIRE(stflString.plaintext() == input);
		}
	}
}

TEST_CASE("Constructing StflRichText from quoted string splits tags from plaintext",
	"[StflRichText]")
{
	const std::map<std::string, std::string> input_vs_plaintext {
		{ "", ""},
		{ "normal text", "normal text" },
		{ "<tag>text</>", "text" },
		{ "abc <tag>text</> def", "abc text def" },
		{ "escaped <>bracket>", "escaped <bracket>" },
		{ "just end tag </> without opening tag", "just end tag  without opening tag" },
		{ "no <color> tag", "no  tag" },
	};

	for (const auto& kv : input_vs_plaintext) {
		const auto& input = kv.first;
		const auto& expected_plaintext = kv.second;
		DYNAMIC_SECTION("plaintext: " << input) {
			const auto stflString = StflRichText::from_quoted(input);
			REQUIRE(stflString.plaintext() == expected_plaintext);
		}
	}
}

TEST_CASE("Apply overlapping style tags", "[StflRichText]")
{
	// Indices:
	// start: 0
	// a: 4
	// b: 7
	// /: 10
	// end: 24
	auto richtext = StflRichText::from_quoted("text<a>abc<b>def</> and remainder");
	const std::string newTag = "<tag>";

	SECTION("complete string overrides all existing tags") {
		richtext.apply_style_tag(newTag, 0, 24);
		REQUIRE(richtext.stfl_quoted() == "<tag>textabcdef and remainder</>");
	}

	SECTION("empty range is ignored") {
		richtext.apply_style_tag(newTag, 10, 10);
		REQUIRE(richtext.stfl_quoted() == "text<a>abc<b>def</> and remainder");
	}

	SECTION("invalid range is ignored") {
		richtext.apply_style_tag(newTag, 10, 9);
		REQUIRE(richtext.stfl_quoted() == "text<a>abc<b>def</> and remainder");
	}

	SECTION("exact overlap with existing tag ranges") {
		SECTION("a") {
			richtext.apply_style_tag(newTag, 4, 7);
			REQUIRE(richtext.stfl_quoted() == "text<tag>abc<b>def</> and remainder");
		}

		SECTION("b") {
			richtext.apply_style_tag(newTag, 7, 10);
			REQUIRE(richtext.stfl_quoted() == "text<a>abc<tag>def</> and remainder");
		}

		SECTION("a+b") {
			richtext.apply_style_tag(newTag, 4, 10);
			REQUIRE(richtext.stfl_quoted() == "text<tag>abcdef</> and remainder");
		}
	}

	SECTION("overlap with start of existing range") {
		richtext.apply_style_tag(newTag, 0, 6);
		REQUIRE(richtext.stfl_quoted() == "<tag>textab<a>c<b>def</> and remainder");
	}

	SECTION("overlap with end of existing range") {
		richtext.apply_style_tag(newTag, 8, 14);
		REQUIRE(richtext.stfl_quoted() == "text<a>abc<b>d<tag>ef and</> remainder");
	}
}
