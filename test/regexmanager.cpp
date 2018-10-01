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
