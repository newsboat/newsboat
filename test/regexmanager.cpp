#include "regexmanager.h"

#include "3rd-party/catch.hpp"
#include "exceptions.h"

using namespace newsboat;

TEST_CASE("RegexManager throws on invalid `highlight' definition",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	SECTION("on `highlight' without parameters")
	{
		REQUIRE_THROWS_AS(rxman.handle_action("highlight", params),
			ConfigHandlerException);
	}

	SECTION("on invalid location")
	{
		params = {"invalidloc", "foo", "blue", "red"};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight", params),
			ConfigHandlerException);
	}

	SECTION("on invalid regex")
	{
		params = {"feedlist", "*", "blue", "red"};
		REQUIRE_THROWS_AS(rxman.handle_action("highlight", params),
			ConfigHandlerException);
	}

	SECTION("on invalid command")
	{
		REQUIRE_THROWS_AS(
			rxman.handle_action("an-invalid-command", params),
			ConfigHandlerException);
	}
}

TEST_CASE("RegexManager doesn't throw on valid `highlight' definition",
	"[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params;

	params = {"articlelist", "foo", "blue", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

	params = {"feedlist", "foo", "blue", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

	params = {"feedlist", "fbo", "blue", "red", "bold", "underline"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));

	params = {"all", "fba", "blue", "red", "bold", "underline"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));
}

TEST_CASE("RegexManager highlights according to definition", "[RegexManager]")
{
	RegexManager rxman;
	std::string input;

	rxman.handle_action("highlight", {"articlelist", "foo", "blue", "red"});
	input = "xfoox";
	rxman.quote_and_highlight(input, "articlelist");
	REQUIRE(input == "x<0>foo</>x");

	rxman.handle_action("highlight", {"feedlist", "foo", "blue", "red"});
	input = "yfooy";
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "y<0>foo</>y");
}

TEST_CASE("RegexManager preserves text when there's nothing to highlight",
	"[RegexManager]")
{
	RegexManager rxman;
	std::string input = "xbarx";
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "xbarx");

	input = "<";
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "<");

	input = "a<b>";
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "a<b>");
}

TEST_CASE("`highlight all` adds rules for all locations", "[RegexManager]")
{
	RegexManager rxman;
	std::vector<std::string> params = {"all", "foo", "red"};
	REQUIRE_NOTHROW(rxman.handle_action("highlight", params));
	std::string input = "xxfooyy";

	for (auto location : {"article", "articlelist", "feedlist"}) {
		SECTION(location)
		{
			rxman.quote_and_highlight(input, location);
			REQUIRE(input == "xx<0>foo</>yy");
		}
	}
}

TEST_CASE("RegexManager does not hang on regexes that can match empty strings", "[RegexManager]")
{
	RegexManager rxman;
	std::string input = "The quick brown fox jumps over the lazy dog";

	rxman.handle_action("highlight", {"feedlist", "w*", "blue", "red"});
	rxman.quote_and_highlight(input, "feedlist");
	REQUIRE(input == "The quick bro<0>w</>n fox jumps over the lazy dog");
}

TEST_CASE("RegexManager does not hang on regexes that match empty strings", "[RegexManager]")
{
	RegexManager rxman;
	std::string input = "The quick brown fox jumps over the lazy dog";
	const std::string compare = input;

	SECTION("testing end of line empty.")
	{
		rxman.handle_action("highlight", {"feedlist", "$", "blue", "red"});
		rxman.quote_and_highlight(input, "feedlist");
		REQUIRE(input == compare);
	}

	SECTION("testing beginning of line empty")
	{
		rxman.handle_action("highlight", {"feedlist", "^", "blue", "red"});
		rxman.quote_and_highlight(input, "feedlist");
		REQUIRE(input == compare);
	}

	SECTION("testing empty line")
	{
		rxman.handle_action("highlight", {"feedlist", "^$", "blue", "red"});
		rxman.quote_and_highlight(input, "feedlist");
		REQUIRE(input == compare);
	}
}

TEST_CASE("quote_and_highlight wraps highlighted text in numbered tags", "[RegexManager]")
{
	RegexManager rxman;
	std::string input =  "The quick brown fox jumps over the lazy dog";

	SECTION("Beginning of line match first")
	{
		const std::string output = "<0>The</> quick <1>brown</> fox jumps over <0>the</> lazy dog";
		rxman.handle_action("highlight", {"article", "the", "red"});
		rxman.handle_action("highlight", {"article", "brown", "blue"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	SECTION("Beginning of line match second")
	{
		const std::string output = "<1>The</> quick <0>brown</> fox jumps over <1>the</> lazy dog";
		rxman.handle_action("highlight", {"article", "brown", "blue"});
		rxman.handle_action("highlight", {"article", "the", "red"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}

	SECTION("2 non-overlapping highlights")
	{
		const std::string output = "The <0>quick</> <1>brown</> fox jumps over the lazy dog";
		rxman.handle_action("highlight", {"article", "quick", "red"});
		rxman.handle_action("highlight", {"article", "brown", "blue"});
		rxman.quote_and_highlight(input, "article");
		REQUIRE(input == output);
	}
}

TEST_CASE("Extract_outer_marker pulls tags", "[RegexManager]")
{
	RegexManager rxman;
	std::string out;

	SECTION("Find outer tag basic")
	{
		std::string input = "<1>TestString</>";
		out = rxman.extract_outer_marker(input, 7);

		REQUIRE(out == "<1>");
	}

	SECTION("Find nested tag")
	{
		std::string input = "<1>Nested<2>Test</>String</>";
		out = rxman.extract_outer_marker(input, 14);

		REQUIRE(out == "<2>");
	}

	SECTION("Find outer tag with second set")
	{
		std::string input = "<1>Nested<2>Test</>String</>";
		out = rxman.extract_outer_marker(input, 21);

		REQUIRE(out == "<1>");
	}

	SECTION("Find unclosed nested tag")
	{
		std::string input = "<1>Nested<2>Test</>String";
		out = rxman.extract_outer_marker(input, 21);

		REQUIRE(out == "<1>");
	}

}
